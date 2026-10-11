#!/usr/bin/env python3
"""Launch the selected Windows game, or install a per-game Steam wrapper.

The wrapper applies the release environment in Steam's child process. Steam's
existing process environment is deliberately not used to enable the NR layer.
Only LaunchOptions is edited; Steam is asked to exit normally before an edit.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import time
import uuid


class LaunchError(RuntimeError):
    pass


def _core():
    import windows_wizard_core
    return windows_wizard_core


def _emit(emit, message):
    if emit is not None:
        emit(message)


def _key(path):
    return str(Path(path).resolve()).replace("/", "\\").casefold()


def _inside(path, directory):
    key, base = _key(path), _key(directory).rstrip("\\")
    return key == base or key.startswith(base + "\\")


def _work(profile):
    path = Path(profile.root) / "work" / "windows-wizard"
    path.mkdir(parents=True, exist_ok=True)
    return path


def _json_write(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    data = (json.dumps(value, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    _atomic_write(path, path.read_bytes() if path.exists() else None, data)


def _atomic_write(path, expected, data):
    """Replace atomically, checking again after the temporary file is durable."""
    path = Path(path)
    actual = path.read_bytes() if path.exists() else None
    if actual != expected:
        raise LaunchError(f"File changed concurrently; left untouched: {path}")
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + ".nr-",
                                         suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        actual = path.read_bytes() if path.exists() else None
        if actual != expected:
            raise LaunchError(f"File changed concurrently; left untouched: {path}")
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def _quote(value):
    """Always quote one Windows argv item; this is not cmd.exe escaping."""
    value = str(value)
    quoted = '"'
    slashes = 0
    for char in value:
        if char == "\\":
            slashes += 1
            continue
        if char == '"':
            quoted += "\\" * (2 * slashes + 1) + '"'
        else:
            quoted += "\\" * slashes + char
        slashes = 0
    return quoted + "\\" * (2 * slashes) + '"'


def _split_commandline(command):
    """Decode Windows quoting for inspecting existing options, without a shell."""
    args, current = [], []
    quoted = False
    index = 0
    started = False
    while index < len(command):
        char = command[index]
        if char.isspace() and not quoted:
            if started:
                args.append("".join(current))
                current, started = [], False
            index += 1
            continue
        started = True
        if char == "\\":
            end = index
            while end < len(command) and command[end] == "\\":
                end += 1
            count = end - index
            if end < len(command) and command[end] == '"':
                current.append("\\" * (count // 2))
                if count % 2:
                    current.append('"')
                else:
                    quoted = not quoted
                index = end + 1
            else:
                current.append("\\" * count)
                index = end
        elif char == '"':
            quoted = not quoted
            index += 1
        else:
            current.append(char)
            index += 1
    if quoted:
        raise LaunchError("Existing Steam launch options have unmatched quotes.")
    if started:
        args.append("".join(current))
    return args


@dataclass
class _Node:
    key: str
    value: str | list
    start: int
    value_start: int
    end: int
    close: int | None = None


def _vdf(text):
    """Parse KeyValues and retain spans so unrelated bytes stay unchanged."""
    tokens = []
    pos = 0
    while pos < len(text):
        if text[pos].isspace() or text[pos] == "\ufeff":
            pos += 1
        elif text.startswith("//", pos):
            pos = text.find("\n", pos)
            if pos < 0:
                break
        elif text[pos] in "{}":
            tokens.append((text[pos], pos, pos + 1))
            pos += 1
        elif text[pos] == '"':
            start = pos
            pos += 1
            value = []
            while pos < len(text) and text[pos] != '"':
                if text[pos] == "\\" and pos + 1 < len(text):
                    following = text[pos + 1]
                    escapes = {"n": "\n", "r": "\r", "t": "\t", '"': '"', "\\": "\\"}
                    if following in escapes:
                        value.append(escapes[following])
                        pos += 2
                        continue
                value.append(text[pos])
                pos += 1
            if pos >= len(text):
                raise LaunchError("Unterminated quoted string in Steam VDF.")
            pos += 1
            tokens.append(("".join(value), start, pos))
        else:
            start = pos
            while pos < len(text) and not text[pos].isspace() and text[pos] not in '{}"':
                pos += 1
            if start == pos:
                raise LaunchError("Invalid token in Steam VDF.")
            tokens.append((text[start:pos], start, pos))
    cursor = 0

    def block(nested=False):
        nonlocal cursor
        nodes = []
        while cursor < len(tokens):
            key, start, _ = tokens[cursor]
            if key == "}":
                if not nested:
                    raise LaunchError("Unexpected closing brace in Steam VDF.")
                cursor += 1
                return nodes, start
            if key == "{":
                raise LaunchError("Expected a key in Steam VDF.")
            cursor += 1
            if cursor >= len(tokens):
                raise LaunchError("Missing value in Steam VDF.")
            value, value_start, end = tokens[cursor]
            cursor += 1
            if value == "{":
                children, close = block(True)
                nodes.append(_Node(key, children, start, value_start, close + 1, close))
            elif value == "}":
                raise LaunchError("Missing value before closing brace in Steam VDF.")
            else:
                nodes.append(_Node(key, value, start, value_start, end))
        if nested:
            raise LaunchError("Unterminated block in Steam VDF.")
        return nodes, None

    return block()[0]


def _find(nodes, key):
    found = [node for node in nodes if node.key.casefold() == key.casefold()]
    if len(found) > 1:
        raise LaunchError(f"Ambiguous duplicate Steam VDF key: {key}")
    return found[0] if found else None


def _branch(nodes, keys):
    node = None
    for key in keys:
        node = _find(nodes, key)
        if node is None:
            return None
        if not isinstance(node.value, list):
            raise LaunchError(f"Expected Steam VDF block: {key}")
        nodes = node.value
    return node


def _apps(text):
    return _branch(_vdf(text), ("UserLocalConfigStore", "Software", "Valve", "Steam", "apps"))


def _options(data, app_id):
    text = data.decode("utf-8-sig")
    apps = _apps(text)
    if apps is None:
        raise LaunchError("Steam localconfig has no apps block; use manual launch options.")
    app = _find(apps.value, app_id)
    if app is None:
        return None
    if not isinstance(app.value, list):
        raise LaunchError("Steam application entry is not a VDF block.")
    options = _find(app.value, "LaunchOptions")
    if options is None:
        return None
    if not isinstance(options.value, str):
        raise LaunchError("Steam LaunchOptions is not a string.")
    return options.value


def _vdf_quote(value):
    return '"' + str(value).replace("\\", "\\\\").replace('"', '\\"').replace(
        "\r", "\\r").replace("\n", "\\n").replace("\t", "\\t") + '"'


def _edit_options(data, app_id, value):
    bom = b"\xef\xbb\xbf" if data.startswith(b"\xef\xbb\xbf") else b""
    text = data.decode("utf-8-sig")
    apps = _apps(text)
    if apps is None:
        raise LaunchError("Steam localconfig has no apps block; use manual launch options.")
    app = _find(apps.value, app_id)
    newline = "\r\n" if "\r\n" in text else "\n"
    if app is None:
        if value is None:
            return data
        insert = (f'{newline}\t\t\t\t\t{_vdf_quote(app_id)}{newline}'
                  f'\t\t\t\t\t{{{newline}\t\t\t\t\t\t"LaunchOptions"\t\t{_vdf_quote(value)}'
                  f'{newline}\t\t\t\t\t}}{newline}')
        text = text[:apps.close] + insert + text[apps.close:]
    else:
        if not isinstance(app.value, list):
            raise LaunchError("Steam application entry is not a VDF block.")
        option = _find(app.value, "LaunchOptions")
        if option is not None:
            if not isinstance(option.value, str):
                raise LaunchError("Steam LaunchOptions is not a string.")
            if value is None:
                text = text[:option.start] + text[option.end:]
            else:
                text = text[:option.value_start] + _vdf_quote(value) + text[option.end:]
        elif value is not None:
            insert = f'{newline}\t\t\t\t\t\t"LaunchOptions"\t\t{_vdf_quote(value)}{newline}'
            text = text[:app.close] + insert + text[app.close:]
    result = bom + text.encode("utf-8")
    if _options(result, app_id) != value:
        raise LaunchError("Steam VDF edit did not round-trip correctly.")
    return result


def _installation(game_exe):
    game_exe = Path(game_exe).resolve()
    for parent in game_exe.parents:
        if parent.name.casefold() == "common" and parent.parent.name.casefold() == "steamapps":
            relative = game_exe.relative_to(parent)
            if len(relative.parts) >= 2:
                return parent / relative.parts[0], parent.parent
    return game_exe.parent, None


def detect_steam_app_id(game_exe):
    install, steamapps = _installation(game_exe)
    if steamapps is None:
        return None
    matches = []
    for manifest in steamapps.glob("appmanifest_*.acf"):
        try:
            state = _branch(_vdf(manifest.read_text(encoding="utf-8-sig")), ("AppState",))
            app = _find(state.value, "appid") if state else None
            directory = _find(state.value, "installdir") if state else None
            if (app and directory and isinstance(app.value, str) and app.value.isdigit()
                    and isinstance(directory.value, str)
                    and _key(steamapps / "common" / directory.value) == _key(install)):
                matches.append(app.value)
        except (OSError, UnicodeError, LaunchError):
            continue
    return matches[0] if len(set(matches)) == 1 else None


def _registry(name, subkey=r"Software\Valve\Steam"):
    if os.name != "nt":
        return None
    import winreg
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, subkey) as key:
            return winreg.QueryValueEx(key, name)[0]
    except OSError:
        return None


def _steam_exe(given=None):
    candidates = [Path(given)] if given else []
    registered = _registry("SteamExe")
    if registered:
        candidates.append(Path(registered))
    registered = _registry("SteamPath")
    if registered:
        candidates.append(Path(registered) / "steam.exe")
    if os.environ.get("ProgramFiles(x86)"):
        candidates.append(Path(os.environ["ProgramFiles(x86)"]) / "Steam" / "steam.exe")
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise LaunchError("Steam executable not found; use manual launch options.")


def _localconfig(steam_exe):
    userdata = Path(steam_exe).parent / "userdata"
    active = _registry("ActiveUser", r"Software\Valve\Steam\ActiveProcess")
    if active:
        path = userdata / str(active) / "config" / "localconfig.vdf"
        if path.is_file():
            return path
        raise LaunchError("Active Steam account has no localconfig.vdf; use manual launch options.")
    choices = list(userdata.glob("*/config/localconfig.vdf"))
    if len(choices) == 1:
        return choices[0]
    raise LaunchError("Cannot choose the active Steam account; use manual launch options.")


def _processes(timeout=15):
    if os.name != "nt":
        raise LaunchError("Windows process inspection is required for safe game/Steam launch.")
    powershell = Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32/WindowsPowerShell/v1.0/powershell.exe"
    script = ("[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false); "
              "$ErrorActionPreference='Stop'; "
              "@(Get-CimInstance Win32_Process | Select-Object ProcessId,Name,ExecutablePath) "
              "| ConvertTo-Json -Compress")
    result = subprocess.run([str(powershell), "-NoProfile", "-NonInteractive", "-Command", script],
                            capture_output=True, text=True, encoding="utf-8", errors="replace",
                            timeout=timeout, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    if result.returncode:
        raise LaunchError("Cannot inspect active Windows processes safely: " + result.stderr.strip())
    parsed = json.loads(result.stdout.strip() or "[]")
    return parsed if isinstance(parsed, list) else [parsed]


def active_game_processes(profile):
    """Check both this game and the preceding game sharing this root's pipe."""
    install, _ = _installation(profile.game_exe)
    installations = [install]
    names = {Path(profile.game_exe).name.casefold()}
    previous_pid = None
    state = Path(profile.root) / "work/windows-wizard/launch-state.json"
    if state.exists():
        try:
            previous = json.loads(state.read_text(encoding="utf-8"))
            if not isinstance(previous, dict):
                raise LaunchError("Previous launch state is not a JSON object.")
            if not previous.get("root") or _key(previous["root"]) != _key(profile.root):
                raise LaunchError("Previous launch state belongs to another release root.")
            if previous.get("game_exe"):
                directory, _ = _installation(previous["game_exe"])
                installations.append(directory)
                names.add(Path(previous["game_exe"]).name.casefold())
                previous_pid = previous.get("pid")
        except (OSError, ValueError, TypeError) as error:
            raise LaunchError(f"Cannot inspect previous launch state safely: {error}") from error
    running = []
    for process in _processes():
        if process.get("ProcessId") == os.getpid():
            continue
        executable = process.get("ExecutablePath")
        name = str(process.get("Name", "")).casefold()
        if ((executable and any(_inside(executable, directory) for directory in installations))
                or (not executable and name in names)
                or (process.get("ProcessId") == previous_pid and name in names)):
            running.append(process)
    return running


def _steam_games(steam_exe, profile):
    libraries = {Path(steam_exe).parent / "steamapps" / "common"}
    install, _ = _installation(profile.game_exe)
    libraries.add(install)
    folders = Path(steam_exe).parent / "steamapps/libraryfolders.vdf"
    if folders.is_file():
        try:
            def walk(nodes):
                for node in nodes:
                    if isinstance(node.value, list):
                        yield from walk(node.value)
                    elif node.key.casefold() == "path":
                        yield Path(node.value) / "steamapps/common"
            libraries.update(walk(_vdf(folders.read_text(encoding="utf-8-sig"))))
        except (OSError, UnicodeError, LaunchError) as error:
            raise LaunchError(f"Cannot inspect Steam libraries safely: {error}") from error
    processes = _processes()
    games = [process for process in processes if process.get("ExecutablePath")
             and any(_inside(process["ExecutablePath"], folder) for folder in libraries)]
    running = _registry("RunningAppID")
    if running and str(running) != "0":
        games.append({"Name": f"Steam RunningAppID {running}", "ProcessId": None})
    return games


def _steam_running(timeout=15):
    return any(str(process.get("Name", "")).casefold() == "steam.exe"
               for process in _processes(timeout=timeout))


def _stop_steam(steam_exe, profile, emit):
    games = _steam_games(steam_exe, profile)
    if games and all(game.get("ProcessId") is None for game in games):
        # Steam keeps RunningAppID for a few seconds after a game exits. With no game process
        # left in its libraries, wait for that to clear rather than refuse Restore Steam.
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            time.sleep(0.5)
            running = _registry("RunningAppID")
            if not running or str(running) == "0":
                games = []
                break
    if games:
        raise LaunchError("A Steam game is running; launch options were left unchanged. Close the game first.")
    running = _steam_running()
    if not running:
        return False
    _emit(emit, "Asking Steam to exit normally before updating this game's launch options.")
    subprocess.Popen([str(steam_exe), "-shutdown"], stdin=subprocess.DEVNULL,
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                     creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    deadline = time.monotonic() + 20
    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        if not _steam_running(timeout=min(2, remaining)):
            return True
        time.sleep(min(0.25, max(0, deadline - time.monotonic())))
    raise LaunchError("Steam did not exit within 20 seconds; nothing was edited and no process was killed.")


def _restart_steam(steam_exe):
    environment = os.environ.copy()
    for key in list(environment):
        if (key.startswith(("NR_", "XMX_")) or key in ("ENABLE_NR_LAYER", "DISABLE_NR_LAYER",
                "DISABLE_VK_LAYER_VALVE_steam_fossilize_1")):
            environment.pop(key, None)
    for key in ("VK_LAYER_PATH", "VK_ADD_IMPLICIT_LAYER_PATH", "VK_INSTANCE_LAYERS"):
        if "dlss" in environment.get(key, "").casefold():
            environment.pop(key, None)
    process = subprocess.Popen([str(steam_exe), "-silent"], env=environment,
                               stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL,
                               creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    return process.pid


def _wrapper_path(profile):
    installed = Path(profile.root) / "scripts/windows_launch.py"
    if installed.is_file():
        return installed.resolve()
    source = Path(profile.root) / "src/tools/windows_launch.py"
    if source.is_file():
        return source.resolve()
    raise LaunchError("The release has no Windows Steam wrapper script.")


def _steam_profile_path(profile):
    serialized = json.dumps(profile.to_dict(), sort_keys=True, ensure_ascii=False,
                            separators=(",", ":")).encode("utf-8")
    digest = hashlib.sha256(serialized).hexdigest()
    return Path(profile.root) / "work/windows-wizard/steam-profiles" / (digest + ".json")


def _ensure_steam_profile(profile):
    """Installed wrappers keep their profile even when the GUI edits its draft."""
    path = _steam_profile_path(profile)
    expected = profile.to_dict()
    if path.exists():
        if json.loads(path.read_text(encoding="utf-8")) != expected:
            raise LaunchError("Steam wrapper profile was modified; the existing file was preserved.")
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        data = (json.dumps(expected, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
        _atomic_write(path, None, data)
    return path


def _steam_python(profile):
    """Hide only the wrapper console; the daemon retains the chosen interpreter."""
    candidate = Path(profile.python).with_name("pythonw.exe")
    try:
        info = candidate.lstat()
        if (stat.S_ISREG(info.st_mode) and not candidate.is_symlink()
                and not getattr(candidate, "is_junction", lambda: False)()
                and not (getattr(info, "st_file_attributes", 0)
                         & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0))
                and _core().pe_architecture(candidate) == "x64"):
            return candidate
    except OSError:
        pass
    return Path(profile.python)


def steam_launch_options(profile):
    return " ".join((_quote(_steam_python(profile)), _quote(_wrapper_path(profile)), "--steam-wrapper",
                     _quote(_steam_profile_path(profile)), "--", "%command%"))


def _ordinary_options(options):
    if not options or not options.strip():
        return True
    if "%command%" in options.casefold() or any(char in options for char in "\r\n&|<>"):
        return False
    args = _split_commandline(options)
    return not args or args[0].startswith(("-", "+"))


def _manual(profile, reason):
    try:
        _ensure_steam_profile(profile)
        options = steam_launch_options(profile)
    except (OSError, ValueError, LaunchError):
        options = None
    return {"ok": False, "manual": True, "error": str(reason), "launch_options": options,
            "original_options_preserved": True}


def configure_steam(profile, steam_exe=None, emit=None):
    """Steam's launch options for the game set to start it through main() below.

    Setup no longer offers this: vulkan-1.dll beside the game (nr_vulkan_proxy.c) gives it
    NR's environment however it is started. Kept with restore_steam and main() for the
    launch options earlier setups left, which Install and Remove NR return."""
    restarted = False
    executable = None
    result = {}
    updated = False
    try:
        app_id = detect_steam_app_id(profile.game_exe)
        if not app_id:
            raise LaunchError("Cannot identify the selected executable's installed Steam app.")
        requested_id = str(getattr(profile, "steam_app_id", "") or "")
        if requested_id and requested_id != app_id:
            raise LaunchError("Profile Steam app ID does not match the installed game's manifest.")
        executable = _steam_exe(steam_exe)
        config = _localconfig(executable)
        backup_path = _work(profile) / "steam-backup.json"
        initial = config.read_bytes()
        original = _options(initial, app_id)
        wrapper = steam_launch_options(profile)
        if backup_path.exists():
            backup = json.loads(backup_path.read_text(encoding="utf-8"))
            if not isinstance(backup, dict) or backup.get("format") != 1:
                raise LaunchError("Steam backup has an unsupported format; it was preserved.")
            if not backup.get("restored", False):
                if (backup.get("root") == _key(profile.root) and backup.get("app_id") == app_id
                        and backup.get("localconfig") == str(config)
                        and original == backup.get("installed_launch_options")
                        and backup.get("profile_snapshot") == str(_steam_profile_path(profile))):
                    _ensure_steam_profile(profile)
                    result = {"ok": True, "already_configured": True, "app_id": app_id,
                              "launch_options": original, "backup": str(backup_path)}
                    return result
                raise LaunchError("An unresolved Steam backup exists; it was preserved. Restore it before reconfiguring.")
        if not _ordinary_options(original):
            raise LaunchError("Existing Steam options contain a custom wrapper or shell command; they were preserved. Use manual configuration.")
        snapshot = _ensure_steam_profile(profile)
        installed = wrapper + (" " + original if original else "")
        restarted = _stop_steam(executable, profile, emit)
        before = config.read_bytes()
        if _options(before, app_id) != original:
            raise LaunchError("Steam launch options changed while stopping Steam; they were left untouched.")
        after = _edit_options(before, app_id, installed)
        backup = {"format": 1, "root": _key(profile.root), "app_id": app_id,
                  "localconfig": str(config), "steam_exe": str(executable),
                  "profile_snapshot": str(snapshot), "game_exe": str(profile.game_exe),
                  "original_launch_options": original, "installed_launch_options": installed,
                  "original_file_sha256": hashlib.sha256(before).hexdigest(),
                  "prepared_utc": datetime.now(timezone.utc).isoformat(), "restored": False}
        _json_write(backup_path, backup)
        _atomic_write(config, before, after)
        updated = True
        backup["configured"] = True
        _json_write(backup_path, backup)
        _emit(emit, "Steam wrapper installed; existing game arguments were preserved.")
        result = {"ok": True, "configured": True, "app_id": app_id, "launch_options": installed,
                  "localconfig": str(config), "backup": str(backup_path),
                  "profile_snapshot": str(snapshot), "original_launch_options": original}
        return result
    except (OSError, UnicodeError, ValueError, LaunchError, subprocess.SubprocessError) as error:
        result = _manual(profile, error)
        if updated:
            result.update(configured=True, original_options_preserved=False,
                          backup=str(backup_path))
        return result
    finally:
        if restarted and executable is not None:
            result["steam_shutdown_command"] = [str(executable), "-shutdown"]
            result["steam_restart_command"] = [str(executable), "-silent"]
            try:
                result["steam_restart_pid"] = _restart_steam(executable)
            except (OSError, subprocess.SubprocessError) as error:
                result.update(ok=False, steam_restart_error=str(error),
                              error="Steam options were handled, but Steam could not restart: " + str(error))


def restore_steam(profile, emit=None):
    restarted = False
    executable = None
    result = {}
    updated = False
    try:
        backup_path = _work(profile) / "steam-backup.json"
        if not backup_path.is_file():
            return {"ok": True, "already_restored": True, "changed": False}
        backup = json.loads(backup_path.read_text(encoding="utf-8"))
        if (not isinstance(backup, dict) or backup.get("root") != _key(profile.root)
                or backup.get("format") != 1):
            raise LaunchError("Steam backup belongs to a different release; it was preserved.")
        if backup.get("restored"):
            return {"ok": True, "already_restored": True, "changed": False}
        config = Path(backup["localconfig"])
        app_id = backup["app_id"]
        original = backup.get("original_launch_options")
        current = _options(config.read_bytes(), app_id)
        if current == original:
            # Recover an interrupted edit whose VDF commit succeeded before its
            # backup metadata could be written (or whose commit never started).
            backup["restored"] = True
            backup["restored_utc"] = datetime.now(timezone.utc).isoformat()
            _json_write(backup_path, backup)
            result = {"ok": True, "already_restored": True, "changed": False,
                      "app_id": app_id, "backup": str(backup_path), "metadata_recovered": True}
            return result
        if current != backup.get("installed_launch_options"):
            raise LaunchError("Steam launch options no longer match this wizard's wrapper; the user's current options were preserved.")
        executable = _steam_exe(Path(backup["steam_exe"]))
        restarted = _stop_steam(executable, profile, emit)
        before = config.read_bytes()
        if _options(before, app_id) != backup["installed_launch_options"]:
            raise LaunchError("Steam launch options changed while stopping Steam; they were left untouched.")
        _atomic_write(config, before, _edit_options(before, app_id, original))
        updated = True
        backup["restored"] = True
        backup["restored_utc"] = datetime.now(timezone.utc).isoformat()
        _json_write(backup_path, backup)
        _emit(emit, "Original Steam launch options restored.")
        result = {"ok": True, "changed": True, "app_id": app_id,
                  "launch_options": original, "backup": str(backup_path)}
        return result
    except (OSError, UnicodeError, ValueError, KeyError, LaunchError, subprocess.SubprocessError) as error:
        result = {"ok": False, "error": str(error), "original_options_preserved": not updated,
                  "changed": updated}
        return result
    finally:
        if restarted and executable is not None:
            result["steam_shutdown_command"] = [str(executable), "-shutdown"]
            result["steam_restart_command"] = [str(executable), "-silent"]
            try:
                result["steam_restart_pid"] = _restart_steam(executable)
            except (OSError, subprocess.SubprocessError) as error:
                result.update(ok=False, steam_restart_error=str(error),
                              error="Steam options were handled, but Steam could not restart: " + str(error))


def _ready(profile):
    checked = _core().validate(profile)
    if not checked.get("ok"):
        raise LaunchError(checked.get("error") or "The selected release/game/Python did not validate.")
    marker = Path(profile.game_exe).parent / "dlss-nr/.windows-wizard-owner.json"
    try:
        owner = json.loads(marker.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise LaunchError("Install this profile's layer before launching the game.") from error
    if (not isinstance(owner, dict) or owner.get("schema_version") != _core().SCHEMA_VERSION
            or not owner.get("root")
            or _key(owner["root"]) != _key(profile.root)):
        raise LaunchError("The installed game layer belongs to another release.")
    games = active_game_processes(profile)
    if games:
        raise LaunchError("A game using this release is already running; its trigger and launch state were preserved.")


def _launch_direct(profile, emit=None, original_command=None):
    _ready(profile)
    tail = []
    original_exe = None
    if original_command is not None:
        if not original_command:
            raise LaunchError("Steam did not provide its original game command after --.")
        original_exe = Path(original_command[0])
        install, _ = _installation(profile.game_exe)
        if (not original_exe.is_file() or original_exe.suffix.casefold() != ".exe"
                or not _inside(original_exe, install)):
            raise LaunchError("Steam's original executable is outside the selected game's installation; no wrapper was executed.")
        tail = list(original_command[1:])
    arguments = list(getattr(profile, "game_args", []) or [])
    if not all(isinstance(item, str) and "\0" not in item for item in tail + arguments):
        raise LaunchError("Game arguments must be strings without NUL characters.")
    environment = _core().runtime_env(profile)
    if environment.get("NR_PYTHON") != str(profile.python):
        raise LaunchError("Runtime Python does not match the user's selected interpreter.")
    switched = _core().set_effect(profile, False)
    if not switched.get("ok"):
        raise LaunchError(switched.get("error") or "Could not start with the effect off.")
    directory = _work(profile)
    token = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S") + "-" + uuid.uuid4().hex[:8]
    stdout_path = directory / (token + ".game.stdout.log")
    stderr_path = directory / (token + ".game.stderr.log")
    command = [str(profile.game_exe), *tail, *arguments]
    daemon_log = Path(environment["NR_LAYER_LOG"])
    daemon_offset = daemon_log.stat().st_size if daemon_log.exists() else 0
    with stdout_path.open("xb") as stdout, stderr_path.open("xb") as stderr:
        process = subprocess.Popen(command, cwd=str(Path(profile.game_exe).parent), env=environment,
                                   stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr, shell=False)
    state = {"ok": True, "mode": "steam-wrapper" if original_command is not None else "direct",
             "pid": process.pid, "command": command, "command_display": subprocess.list2cmdline(command),
             "stdout_log": str(stdout_path), "stderr_log": str(stderr_path),
             "daemon_log": str(daemon_log), "daemon_start_bytes": daemon_offset,
             "root": str(profile.root), "game_exe": str(profile.game_exe),
             "environment": {key: value for key, value in environment.items()
                             if key.startswith(("NR_", "VK_", "ENABLE_NR", "DISABLE_VK_LAYER_VALVE"))},
             "trigger_initially_exists": False, "started_utc": datetime.now(timezone.utc).isoformat(),
             "exit_code": process.poll()}
    if state["exit_code"] not in (None, 0):
        state.update(ok=False, error=f"Game exited immediately with code {state['exit_code']}.")
    if original_exe is not None:
        state["steam_original_exe"] = str(original_exe)
        state["executable_substituted"] = _key(original_exe) != _key(profile.game_exe)
    _json_write(directory / "launch-state.json", state)
    _emit(emit, "Game launched with the effect off: " + str(profile.game_exe))
    return state, process


def launch(profile, emit=None):
    """The game started by setup itself, with the effect off, whatever the profile's
    launch_mode says: Steam's Play button and any other launcher get NR through the proxy."""
    try:
        return _launch_direct(profile, emit)[0]
    except (OSError, ValueError, LaunchError, subprocess.SubprocessError) as error:
        return {"ok": False, "error": str(error)}


def main(argv=None):
    argv = sys.argv[1:] if argv is None else list(argv)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--steam-wrapper", required=True, metavar="PROFILE")
    if "--" not in argv:
        parser.error("Steam wrapper requires -- followed by %command%.")
    split = argv.index("--")
    args = parser.parse_args(argv[:split])
    profile = None
    try:
        data = json.loads(Path(args.steam_wrapper).read_text(encoding="utf-8-sig"))
        profile = _core().Profile.from_dict(data)
        expected = _steam_profile_path(profile)
        if _key(args.steam_wrapper) != _key(expected):
            raise LaunchError("Steam wrapper profile does not match its immutable release snapshot.")
        result, process = _launch_direct(profile, original_command=argv[split + 1:])
        code = process.wait()
        result["exit_code"] = code
        result["ended_utc"] = datetime.now(timezone.utc).isoformat()
        _json_write(_work(profile) / "launch-state.json", result)
        return code
    except (OSError, ValueError, LaunchError, subprocess.SubprocessError) as error:
        if sys.stderr is not None:
            print("Windows NR Steam wrapper: " + str(error), file=sys.stderr, flush=True)
        if profile is not None:
            try:
                _json_write(_work(profile) / "steam-wrapper-last-error.json",
                            {"ok": False, "error": str(error), "wrapper_pid": os.getpid(),
                             "profile_snapshot": args.steam_wrapper,
                             "time_utc": datetime.now(timezone.utc).isoformat()})
            except (OSError, LaunchError):
                pass
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
