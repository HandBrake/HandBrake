"""CPU-only release fixtures; no game, Steam, vendor weights or GPU execution."""
from dataclasses import replace
import ast
import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from types import SimpleNamespace
from unittest.mock import patch

import windows_wizard_core as core


def write_pe(path, x64=True, suffix=b""):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    data = bytearray(90)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 60, 64)
    data[64:68] = b"PE\0\0"
    struct.pack_into("<H", data, 68, 0x8664 if x64 else 0x14C)
    struct.pack_into("<H", data, 88, 0x20B if x64 else 0x10B)
    path.write_bytes(data + suffix)
    return path


def write_weights(path):
    """Format-only zero-sized tensors; deliberately contain no real weights."""
    path.parent.mkdir(parents=True, exist_ok=True)
    header = json.dumps({f"fixture_{n}": {"dtype": "F16", "shape": [0],
                                         "data_offsets": [0, 0]} for n in range(649)}).encode()
    path.write_bytes(len(header).to_bytes(8, "little") + header)


def python_info(**changes):
    result = {"probe_ok": True, "platform": "win32", "os_name": "nt", "bits": 64,
              "abi_platform": "win-amd64", "native_apis": True, "version": "3.12.10",
              "packages": {"numpy": "fixture", "safetensors": "fixture"},
              "package_errors": {}, "output": ""}
    result.update(changes)
    return result


class WizardFixture(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="nr wizard (fixture) ")
        self.addCleanup(self.temporary.cleanup)
        # Resolved, as the wizard resolves what it is given: a TEMP in 8.3 form
        # (C:\Users\NAME~1\...) otherwise compares unequal to the same file's long name.
        self.base = Path(self.temporary.name).resolve()
        self.root = self.base / "release (candidate)"
        self.root.mkdir()
        for relative in core.RUNTIME_MODULES:
            path = self.root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("# release fixture\n", encoding="utf-8")
        for directory in core.SOURCE_DIRS:
            (self.root / "src" / directory).mkdir(parents=True, exist_ok=True)
        write_pe(self.root / "nr_layer.dll")
        write_pe(self.root / "nr_layer32.dll", x64=False)
        write_pe(self.root / "nr_vulkan_proxy.dll", suffix=b"proxy x64")
        write_pe(self.root / "nr_vulkan_proxy32.dll", x64=False, suffix=b"proxy x32")
        for directory, x64 in (("x64", True), ("x32", False)):
            for name in core.DXVK_FILES:
                write_pe(self.root / "dxvk" / directory / name, x64=x64, suffix=f"DXVK {directory} {name}".encode())
        for name in core.LIBRARIES:
            write_pe(self.root / "work" / name)
        for name in core.SHADERS:
            (self.root / "work" / name).write_bytes(b"SPIR-V fixture")
        core._atomic_json(self.root / "VkLayer_dlss_nr.json", {
            "file_format_version": "1.0.0", "layer": {"name": "VK_LAYER_dlssnr_intel",
            "type": "GLOBAL", "library_path": "nr_layer.dll", "api_version": "1.3.0"}})
        self.game = write_pe(self.base / "game (test)" / "selected.exe")
        self.python = write_pe(self.base / "Python (selected)" / "python.exe")
        self.profile = core.Profile(self.root, self.python, self.game)
        self.weights = self.root / "work/mlxw/dlssnr-logical.safetensors"
        write_weights(self.weights)
        self.native = patch.object(core, "_python_info", return_value=python_info()).start()
        self.deps = patch.object(core, "_load_dependencies", return_value={"ok": True, "failures": {}}).start()
        self.processes = patch.object(core, "_process_snapshot", return_value=[]).start()
        self.running = patch.object(core, "_running_executable", return_value=None).start()
        self.addCleanup(patch.stopall)
        core._LOG_STATES.clear()
        self.addCleanup(core._LOG_STATES.clear)

    def checks(self, profile=None):
        return {item["name"]: item for item in core.validate(profile or self.profile)["checks"]}

    def own(self, profile=None):
        profile = profile or self.profile
        directory = core.installed_path(profile)
        directory.mkdir(parents=True, exist_ok=True)
        core._atomic_json(directory / core.OWNER_NAME, {"schema_version": 1, "root": str(profile.root)})
        return directory

    def launch_state(self, profile=None, pid=123, offset=0, started="fixture-session"):
        profile = profile or self.profile
        core._atomic_json(profile.root / "work/windows-wizard/launch-state.json", {
            "root": str(profile.root), "game_exe": str(profile.game_exe), "pid": pid,
            "daemon_start_bytes": offset, "started_utc": started, "exit_code": None})
        self.processes.return_value = [{"ProcessId": pid, "ExecutablePath": str(profile.game_exe)}]
        self.running.side_effect = lambda value, pid=pid, path=str(profile.game_exe): path if value == pid else None

    def test_profile_defaults_round_trip_and_paths(self):
        minimal = core.Profile.from_dict({"root": str(self.root), "python": str(self.python),
                                         "game_exe": str(self.game)})
        self.assertEqual(minimal.api, "vulkan")
        self.assertEqual(minimal.game_args, [])
        self.assertIsNone(minimal.dll)
        self.assertEqual(core.Profile.from_dict(minimal.to_dict()), minimal)
        self.assertTrue(core.save_profile(minimal)["ok"])
        self.assertEqual(core.load_profile(self.root), minimal)

    def test_steam_blank_app_id_allows_autodetection_and_invalid_id_is_rejected(self):
        for app_id, expected in (("", True), ("379720", True), ("DOOM", False), ("379720 extra", False)):
            with self.subTest(app_id=app_id):
                profile = replace(self.profile, launch_mode="steam", steam_app_id=app_id)
                result = core.validate(profile)
                self.assertEqual(result["ok"], expected)
                self.assertEqual(self.checks(profile)["launch_mode"]["ok"], expected)
        profile = replace(self.profile, launch_mode="steam", steam_app_id="")
        result = core.install(profile)
        self.assertTrue(result["ok"], result)
        self.assertEqual(core.load_profile(self.root).steam_app_id, "")

    def test_runtime_env_same_python_unique_pipe_and_clean_inherited_layers(self):
        with patch.dict(os.environ, {"NR_ROOT": "stale", "NR_PYTHON": "stale", "XMX_TEST": "1",
                                     "VK_LAYER_PATH": "stale", "VK_DEVICE_LAYERS": "stale",
                                     "DISABLE_NR_LAYER": "1", "SteamAppId": "2280"}):
            env = core.runtime_env(self.profile)
        self.assertEqual(env["NR_PYTHON"], str(self.python))
        self.assertEqual(env["NR_ROOT"], str(self.root))
        self.assertEqual(env["NR_DAEMON"], str(self.root / "src/layer/nr_daemon.py"))
        self.assertEqual(env["NR_SETTINGS"], str(self.root / "work/nr_settings.json"))
        self.assertEqual(env["NR_LAYER_TRIGGER"], str(self.root / "work/nr_trigger"))
        self.assertEqual({key.upper(): value for key, value in env.items()}["STEAMAPPID"], "2280")
        for name in ("XMX_TEST", "VK_DEVICE_LAYERS", "DISABLE_NR_LAYER"):
            self.assertNotIn(name, env)
        self.assertEqual(env["VK_LAYER_PATH"], str(self.game.parent / "dlss-nr"))
        self.assertNotEqual(env["NR_LAYER_SOCKET"], core.runtime_env(replace(self.profile, root=self.base / "other"))["NR_LAYER_SOCKET"])
        self.assertEqual(env["NR_LAYER_SOCKET"], core.runtime_env(self.profile)["NR_LAYER_SOCKET"])
        self.assertNotIn("DISABLE_VK_LAYER_VALVE_steam_overlay_1", env)
        self.assertNotIn("DISABLE_VK_LAYER_VALVE_steam_fossilize_1", core.runtime_env(replace(self.profile, disable_fossilize=False)))

    def test_32bit_game_accepted_and_other_machines_and_32bit_python_rejected(self):
        write_pe(self.game, x64=False)
        self.assertTrue(self.checks()["game_architecture"]["ok"])
        arm64 = bytearray(write_pe(self.game).read_bytes())
        struct.pack_into("<H", arm64, 68, 0xAA64)
        self.game.write_bytes(bytes(arm64))
        self.assertFalse(self.checks()["game_architecture"]["ok"])
        write_pe(self.game)
        self.native.return_value = python_info(bits=32, abi_platform="win32")
        self.assertFalse(self.checks()["python_native_x64"]["ok"])
        self.assertFalse(core.install(self.profile)["ok"])
        self.assertFalse(core.installed_path(self.profile).exists())

    def test_mingw_abi_or_missing_native_apis_are_rejected(self):
        for change in ({"abi_platform": "mingw_x86_64"}, {"native_apis": False}):
            with self.subTest(change=change):
                self.native.return_value = python_info(**change)
                self.assertFalse(self.checks()["python_native_x64"]["ok"])

    def test_discovery_excludes_mingw_even_when_inherited_nr_python(self):
        mingw = write_pe(self.base / "mingw Python" / "python.exe")
        self.native.side_effect = lambda candidate, **kwargs: python_info(abi_platform="mingw_x86_64") if Path(candidate) == mingw else python_info()
        with patch.dict(os.environ, {"NR_PYTHON": str(mingw)}), patch.object(core.sys, "executable", str(self.python)), patch.object(core.shutil, "which", return_value=None):
            found = core.discover_python()
        self.assertIn(str(self.python), found)
        self.assertNotIn(str(mingw), found)

    def test_every_runtime_shader_and_allocator_must_be_regular_file(self):
        for relative in ("work/gemm_staged.spv", "work/half_probe.spv", "work/libnr_alloc.dll"):
            with self.subTest(relative=relative):
                path = self.root / relative
                original = path.read_bytes()
                path.unlink()
                path.mkdir()
                self.assertFalse(self.checks()["runtime_files"]["ok"])
                self.assertFalse(core.install(self.profile)["ok"])
                self.assertFalse(core.installed_path(self.profile).exists())
                path.rmdir()
                path.write_bytes(original)

    def test_loader_or_runtime_dependency_failure_is_preflight_failure(self):
        self.deps.return_value = {"ok": False, "failures": {"vulkan-1.dll": "missing loader"}}
        result = core.install(self.profile)
        self.assertFalse(result["ok"])
        self.assertFalse(core.installed_path(self.profile).exists())
        self.assertIn("missing loader", self.checks()["runtime_dependencies"]["detail"])

    def test_source_directory_missing_is_rejected_before_game_staging(self):
        (self.root / "src/bench").rmdir()
        self.assertFalse(self.checks()["runtime_files"]["ok"])
        with patch.object(core.tempfile, "mkdtemp", side_effect=AssertionError("No game writes after failed preflight")):
            self.assertFalse(core.install(self.profile)["ok"])

    def test_dxvk_comes_from_the_release_for_the_games_architecture(self):
        profile = replace(self.profile, api="dxvk")
        self.assertTrue(self.checks(profile)["dxvk_files"]["ok"])
        write_pe(self.root / "dxvk/x64/dxgi.dll", x64=False)
        self.assertFalse(self.checks(profile)["dxvk_files"]["ok"])
        write_pe(self.game, x64=False)
        self.assertTrue(self.checks(profile)["dxvk_files"]["ok"])

    def test_dxvk_install_sets_the_games_files_aside_and_remove_puts_them_back(self):
        profile = replace(self.profile, api="dxvk")
        game = self.game.parent
        (game / "dxgi.dll").write_bytes(b"the game's own proxy")
        shutil.copy2(self.root / "dxvk/x64/d3d9.dll", game / "d3d9.dll")  # someone's identical copy
        (game / f"{self.game.stem}_d3d9.log").write_text("that DXVK's own log")
        self.assertIn("dxgi.dll", self.checks(profile)["game_files"]["detail"])
        result = core.install(profile)
        self.assertTrue(result["ok"], result)
        for name in core.DXVK_FILES:
            self.assertEqual((game / name).read_bytes(), (self.root / "dxvk/x64" / name).read_bytes())
        installed = core.installed_path(profile)
        self.assertEqual((installed / core.GAME_BACKUP / "dxgi.dll").read_bytes(), b"the game's own proxy")
        manifest = json.loads((installed / "VkLayer_dlss_nr.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["layer"]["library_arch"], "64")
        self.assertEqual(Path(core.runtime_env(profile)["DXVK_LOG_PATH"]), self.root / "work/logs")
        # Installed again, the game's original is still the one set aside, once.
        self.assertTrue(core.install(profile)["ok"])
        self.assertEqual((installed / core.GAME_BACKUP / "dxgi.dll").read_bytes(), b"the game's own proxy")
        self.assertFalse((installed / ".undo").exists())
        (game / f"{self.game.stem}_dxgi.log").write_text("a DXVK log from a launch outside NR")
        removed = core.uninstall(profile)
        self.assertTrue(removed["ok"], removed)
        self.assertEqual((game / "dxgi.dll").read_bytes(), b"the game's own proxy")
        self.assertEqual((game / "d3d9.dll").read_bytes(), (self.root / "dxvk/x64/d3d9.dll").read_bytes())
        for name in ("d3d8.dll", "d3d10core.dll", "d3d11.dll", f"{self.game.stem}_dxgi.log"):
            self.assertFalse((game / name).exists(), name)
        self.assertFalse(installed.exists())
        self.assertEqual((game / f"{self.game.stem}_d3d9.log").read_text(), "that DXVK's own log")
        self.assertEqual(sorted(path.name for path in game.iterdir()),
                         sorted(["d3d9.dll", "dxgi.dll", self.game.name, f"{self.game.stem}_d3d9.log"]))

    def test_dxvk_placement_failure_puts_the_games_files_back(self):
        profile = replace(self.profile, api="dxvk")
        game = self.game.parent
        (game / "dxgi.dll").write_bytes(b"original")
        copy = shutil.copy2

        def failing(source, target, *args, **kwargs):
            if Path(source).name == "dxgi.dll":
                raise OSError("disk full")
            return copy(source, target, *args, **kwargs)
        with patch.object(core.shutil, "copy2", side_effect=failing):
            self.assertFalse(core.install(profile)["ok"])
        self.assertEqual(sorted(path.name for path in game.iterdir()), ["dxgi.dll", self.game.name])
        self.assertEqual((game / "dxgi.dll").read_bytes(), b"original")

    def test_late_failure_after_dxvk_commit_puts_the_games_files_back(self):
        profile = replace(self.profile, api="dxvk")
        game = self.game.parent
        (game / "dxgi.dll").write_bytes(b"original")
        with patch.object(core, "save_profile", side_effect=OSError("profile write failed")):
            self.assertFalse(core.install(profile)["ok"])
        self.assertEqual(sorted(path.name for path in game.iterdir()), ["dxgi.dll", self.game.name])
        self.assertEqual((game / "dxgi.dll").read_bytes(), b"original")

    def test_32bit_game_gets_the_32bit_layer_and_dxvk(self):
        write_pe(self.game, x64=False)
        profile = replace(self.profile, api="dxvk")
        self.assertTrue(core.install(profile)["ok"])
        installed = core.installed_path(profile)
        manifest = json.loads((installed / "VkLayer_dlss_nr.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["layer"]["library_arch"], "32")
        self.assertEqual(Path(manifest["layer"]["library_path"]), installed / "nr_layer32.dll")
        self.assertEqual(core.pe_architecture(installed / "nr_layer32.dll"), "x86")
        self.assertEqual((self.game.parent / "d3d9.dll").read_bytes(), (self.root / "dxvk/x32/d3d9.dll").read_bytes())
        self.assertEqual((self.game.parent / core.PROXY_NAME).read_bytes(),
                         (self.root / "nr_vulkan_proxy32.dll").read_bytes())

    def test_every_game_gets_the_proxy_and_its_environment(self):
        game = self.game.parent
        result = core.install(self.profile)
        self.assertTrue(result["ok"], result)
        self.assertEqual((game / core.PROXY_NAME).read_bytes(), (self.root / "nr_vulkan_proxy.dll").read_bytes())
        installed = core.installed_path(self.profile)
        lines = (installed / core.ENV_FILE).read_text(encoding="utf-8").splitlines()
        values = dict(line.split("=", 1) for line in lines if "=" in line and not line.startswith("#"))
        self.assertEqual(Path(values["NR_GAME_EXE"]), self.game)
        self.assertEqual(Path(values["NR_LAUNCH_STATE"]), self.root / "work/windows-wizard/launch-state.json")
        # The proxy makes no folder for its record: without one, status never saw the game.
        self.assertTrue((self.root / "work/windows-wizard").is_dir())
        self.assertNotIn("NR_REAL_VULKAN", values)
        for name, value in core.layer_environment(self.profile).items():
            self.assertEqual(values[name], value, name)
        self.assertTrue(core.uninstall(self.profile)["ok"])
        self.assertEqual(sorted(path.name for path in game.iterdir()), [self.game.name])

    def test_remove_takes_dxvks_first_log_from_the_folders_the_game_started_in(self):
        # DXVK opens its first log before it loads the proxy, so in the folder the game was
        # started in: Steam starts Mortal Kombat 11 in its root, above the executable.
        profile = replace(self.profile, api="dxvk")
        game = self.game.parent
        started = self.base / "started (here)"
        started.mkdir()
        old = started / f"{self.game.stem}_d3d9.log"
        old.write_text("a log from before NR")
        own = game / f"{self.game.stem}_d3d11.log"
        own.write_text("beside the game before NR")
        self.assertTrue(core.install(profile)["ok"])
        installed = core.installed_path(profile)
        since = json.loads((installed / core.GAME_FILES).read_text(encoding="utf-8"))["installed_time"]
        os.utime(old, (since - 60, since - 60))
        (started / f"{self.game.stem}_dxgi.log").write_text("NR's DXVK, before DXVK_LOG_PATH")
        (started / "notes.txt").write_text("not DXVK's")
        own.write_text("beside the game before NR, written again")
        (installed / core.START_FOLDERS).write_text("\n".join(
            [str(started), str(started), str(game), str(self.base / "gone"), "relative", ""]),
            encoding="utf-8")
        removed = core.uninstall(profile)
        self.assertTrue(removed["ok"], removed)
        self.assertEqual(sorted(path.name for path in started.iterdir()), sorted([old.name, "notes.txt"]))
        self.assertEqual(sorted(path.name for path in game.iterdir()), sorted([own.name, self.game.name]))

    def test_a_games_own_vulkan_loader_is_set_aside_and_used(self):
        game = self.game.parent
        (game / core.PROXY_NAME).write_bytes(b"the game's own Vulkan loader")
        self.assertIn(core.PROXY_NAME, self.checks()["game_files"]["detail"])
        self.assertTrue(core.install(self.profile)["ok"])
        installed = core.installed_path(self.profile)
        aside = installed / core.GAME_BACKUP / core.PROXY_NAME
        self.assertEqual(aside.read_bytes(), b"the game's own Vulkan loader")
        self.assertIn("NR_REAL_VULKAN=" + str(aside),
                      (installed / core.ENV_FILE).read_text(encoding="utf-8").splitlines())
        self.assertTrue(core.uninstall(self.profile)["ok"])
        self.assertEqual((game / core.PROXY_NAME).read_bytes(), b"the game's own Vulkan loader")

    def test_the_proxy_must_match_the_games_architecture(self):
        self.assertTrue(self.checks()["runtime_architecture"]["ok"])
        write_pe(self.root / "nr_vulkan_proxy.dll", x64=False)
        self.assertFalse(self.checks()["runtime_architecture"]["ok"])

    def test_changing_the_api_after_dxvk_needs_remove_first(self):
        dxvk = replace(self.profile, api="dxvk")
        self.assertTrue(core.install(dxvk)["ok"])
        self.assertFalse(self.checks(self.profile)["game_files"]["ok"])
        self.assertFalse(core.install(self.profile)["ok"])
        self.assertTrue(core.uninstall(dxvk)["ok"])
        self.assertTrue(core.install(self.profile)["ok"])
        self.assertFalse((self.game.parent / "dxgi.dll").exists())

    def test_remove_leaves_files_changed_since_and_keeps_the_originals(self):
        profile = replace(self.profile, api="dxvk")
        (self.game.parent / "d3d11.dll").write_bytes(b"the game's d3d11")
        self.assertTrue(core.install(profile)["ok"])
        (self.game.parent / "d3d11.dll").write_bytes(b"a game update")
        result = core.uninstall(profile)
        self.assertFalse(result["ok"])
        self.assertEqual(result["kept"], ["d3d11.dll"])
        self.assertEqual((self.game.parent / "d3d11.dll").read_bytes(), b"a game update")
        self.assertFalse((self.game.parent / "dxgi.dll").exists())
        backup = core.installed_path(profile) / core.GAME_BACKUP / "d3d11.dll"
        self.assertEqual(backup.read_bytes(), b"the game's d3d11")

    def test_remove_waits_for_steams_launch_options_to_be_restored(self):
        profile = replace(self.profile, api="dxvk")
        self.assertTrue(core.install(profile)["ok"])
        steam = self.root / "work/windows-wizard/steam-backup.json"
        core._atomic_json(steam, {"restored": False})  # the same, unchanged profile
        self.assertFalse(core.uninstall(profile)["ok"])
        self.assertTrue((self.game.parent / "dxgi.dll").exists())
        core._atomic_json(steam, {"restored": True})
        self.assertTrue(core.uninstall(profile)["ok"])
        self.assertFalse((self.game.parent / "dxgi.dll").exists())

    def test_remove_nr_returns_steams_launch_options_first(self):
        import windows_launch as launch
        import windows_wizard as bridge
        with patch.object(launch, "restore_steam", return_value={"ok": False, "error": "A Steam game is running"}), \
                patch.object(core, "uninstall") as uninstall:
            result = bridge.remove_nr(self.profile, None)
        self.assertEqual(result["error"], "A Steam game is running")
        uninstall.assert_not_called()
        with patch.object(launch, "restore_steam", return_value={"ok": True, "changed": True}) as restore, \
                patch.object(core, "uninstall", return_value={"ok": True, "removed": True}) as uninstall:
            result = bridge.remove_nr(self.profile, None)
        self.assertTrue(result["ok"])
        self.assertTrue(result["steam_restored"])
        self.assertEqual(restore.call_args.args[0], self.profile)
        uninstall.assert_called_once()
        with patch.object(launch, "restore_steam", return_value={"ok": True, "already_restored": True, "changed": False}), \
                patch.object(core, "uninstall", return_value={"ok": True, "removed": True}):
            self.assertNotIn("steam_restored", bridge.remove_nr(self.profile, None))

    def test_enabling_warns_when_the_game_runs_without_nr(self):
        self.own()
        # Started before NR was installed: no launch record names it, the game runs.
        self.processes.return_value = [{"ProcessId": 77, "ExecutablePath": str(self.game)}]
        result = core.set_effect(self.profile, True)
        self.assertTrue(result["ok"])
        self.assertEqual(result.get("warning"), "game_without_nr")
        self.assertNotIn("warning", core.set_effect(self.profile, False))
        # Started through NR, it is not warned about; nor is a game that is not running.
        self.launch_state(pid=77)
        self.assertNotIn("warning", core.set_effect(self.profile, True))
        self.running.side_effect = None
        self.processes.return_value = []
        self.assertNotIn("warning", core.set_effect(self.profile, True))

    def test_remove_refuses_while_the_game_runs_and_without_an_installation(self):
        profile = replace(self.profile, api="dxvk")
        result = core.uninstall(profile)
        self.assertTrue(result["ok"])
        self.assertFalse(result["removed"])
        self.assertTrue(core.install(profile)["ok"])
        self.launch_state()
        self.assertFalse(core.uninstall(profile)["ok"])
        self.assertTrue((self.game.parent / "dxgi.dll").exists())
        self.assertTrue(core.installed_path(profile).exists())

    def test_foreign_existing_installation_is_preserved(self):
        destination = core.installed_path(self.profile)
        destination.mkdir()
        sentinel = destination / "foreign.txt"
        sentinel.write_bytes(b"do not change")
        before = sentinel.read_bytes()
        self.assertFalse(core.install(self.profile)["ok"])
        self.assertEqual(sentinel.read_bytes(), before)
        self.assertEqual(list(destination.iterdir()), [sentinel])
        core._atomic_json(destination / core.OWNER_NAME, {"schema_version": 1, "root": str(self.base / "foreign")})
        self.assertFalse(core.install(self.profile)["ok"])
        self.assertEqual(sentinel.read_bytes(), before)

    def test_successful_install_spaces_parentheses_defaults_and_no_vendor_copy(self):
        trigger = self.root / "work/nr_trigger"
        trigger.touch()
        core._atomic_json(self.root / "work/nr_settings.json", {"render_scale": 0.8, "min_extent": 640, "keep_me": 7})
        result = core.install(self.profile)
        self.assertTrue(result["ok"], result)
        saved = core.load_profile(self.root)
        self.assertEqual(saved.python, self.python)
        self.assertEqual(saved.game_exe, self.game)
        settings = core._read_json(self.root / "work/nr_settings.json")
        self.assertEqual(settings, {"render_scale": 0.8, "min_extent": 640, "keep_me": 7})
        self.assertFalse(trigger.exists())
        destination = core.installed_path(self.profile)
        manifest = core._read_json(destination / "VkLayer_dlss_nr.json")
        self.assertEqual(manifest["layer"]["library_path"], str(destination / "nr_layer.dll"))
        self.assertEqual(manifest["layer"]["library_arch"], "64")
        self.assertFalse(any(destination.rglob("*.safetensors")))
        self.assertFalse(any(destination.rglob("nvngx_dlssnr.dll")))
        self.assertTrue(self.weights.is_file())

    def test_first_install_sets_candidate_defaults_only_when_missing(self):
        result = core.install(self.profile)
        self.assertTrue(result["ok"], result)
        self.assertEqual(core._read_json(self.root / "work/nr_settings.json"),
                         {"render_scale": 0.4, "min_extent": 320})

    def test_control_schema_and_daemon_defaults_come_from_linux_catalogue(self):
        catalogue = core._runtime_knobs()
        self.assertEqual([knob["name"] for knob in catalogue], [
            "render_scale", "min_extent", "profile", "intensity", "detail_strength",
            "colour_strength", "temporal", "hold", "release", "cut_limit"])
        names = {knob["name"]: knob for knob in catalogue}
        self.assertEqual(names["profile"]["choices"], ["standard", "natural", "cinematic", "neutral"])
        self.assertEqual((names["min_extent"]["high"], names["min_extent"]["runtime_high"]), (320, 4096))
        self.assertEqual((names["release"]["high"], names["release"]["runtime_high"]), (64, 255))
        daemon_source = (Path(core.__file__).resolve().parent.parent / "layer/nr_daemon.py").read_text(encoding="utf-8")
        tree = ast.parse(daemon_source)
        actual = {}
        for node in ast.walk(tree):
            if (isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                    and node.func.attr == "add_argument" and node.args
                    and isinstance(node.args[0], ast.Constant)):
                for keyword in node.keywords:
                    if keyword.arg == "default" and isinstance(keyword.value, ast.Constant):
                        actual[str(node.args[0].value).removeprefix("--").replace("-", "_")] = keyword.value.value
        self.assertEqual({knob["name"]: knob["default"] for knob in catalogue},
                         {knob["name"]: actual[knob["name"]] for knob in catalogue})

    def test_reading_control_defaults_needs_no_game_profile_or_processes_and_writes_nothing(self):
        with patch.object(core, "_process_snapshot", side_effect=AssertionError("Controls do not inspect games")), \
                patch.object(core, "_running_executable", side_effect=AssertionError("Controls do not inspect games")):
            result = core.get_settings(self.root)
        self.assertTrue(result["ok"])
        self.assertEqual(result["settings"], result["defaults"])
        self.assertEqual(result["explicit_keys"], [])
        self.assertFalse(core.profile_path(self.root).exists())
        self.assertFalse((self.root / "work/nr_settings.json").exists())
        self.assertFalse((self.root / "work/nr_trigger").exists())

    def test_partial_settings_save_merges_latest_values_and_preserves_trigger_and_profile(self):
        core.save_profile(self.profile)
        profile_before = core.profile_path(self.root).read_bytes()
        trigger = self.root / "work/nr_trigger"
        trigger.write_bytes(b"existing effect")
        path = self.root / "work/nr_settings.json"
        core._atomic_json(path, {"render_scale": 0.4, "min_extent": 640, "release": 100,
                                 "other_tool": {"value": "preserve me"}})
        observed = core.get_settings(self.root)
        self.assertEqual(observed["settings"]["min_extent"], 640)
        external = core._read_json(path)
        external["detail_strength"] = 1.7
        external["other_tool"]["value"] = "new external value"
        core._atomic_json(path, external)
        with patch.object(core, "_process_snapshot", side_effect=AssertionError("No game restart")), \
                patch.object(core, "_running_executable", side_effect=AssertionError("No game restart")):
            result = core.save_settings(self.root, {"render_scale": 0.25})
        saved = core._read_json(path)
        self.assertEqual(saved["render_scale"], 0.25)
        self.assertEqual(saved["detail_strength"], 1.7)
        self.assertEqual(saved["min_extent"], 640)
        self.assertEqual(saved["release"], 100)
        self.assertEqual(saved["other_tool"]["value"], "new external value")
        self.assertFalse(result["game_restart_required"])
        self.assertFalse(result["processing_confirmed"])
        self.assertEqual(trigger.read_bytes(), b"existing effect")
        self.assertEqual(core.profile_path(self.root).read_bytes(), profile_before)

    def test_invalid_runtime_settings_leave_saved_bytes_untouched(self):
        path = self.root / "work/nr_settings.json"
        core._atomic_json(path, {"render_scale": 0.4, "unknown": "untouched"})
        before = path.read_bytes()
        invalid = ({"render_scale": 0.049}, {"render_scale": 1.001}, {"render_scale": True},
                   {"intensity": float("nan")}, {"detail_strength": float("inf")},
                   {"colour_strength": -1}, {"temporal": 1.01}, {"hold": False},
                   {"cut_limit": None}, {"release": 256}, {"min_extent": 127},
                   {"min_extent": 4097}, {"profile": "unknown"}, {"master": True})
        for changes in invalid:
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                core.save_settings(self.root, changes)
            self.assertEqual(path.read_bytes(), before)
        self.assertFalse(list(path.parent.glob(".nr-json-*")))

    def test_saved_invalid_settings_are_reported_instead_of_claiming_default_values(self):
        path = self.root / "work/nr_settings.json"
        for values in ([0.4], {"render_scale": float("nan")}, {"hold": True}):
            with self.subTest(values=values):
                core._atomic_json(path, values)
                with self.assertRaises(ValueError):
                    core.get_settings(self.root)

    def test_atomic_settings_failure_leaves_last_readable_document(self):
        path = self.root / "work/nr_settings.json"
        core._atomic_json(path, {"render_scale": 0.4})
        before = path.read_bytes()
        with patch.object(Path, "replace", side_effect=OSError("fixture rename failure")):
            with self.assertRaises(OSError):
                core.save_settings(self.root, {"render_scale": 0.3})
        self.assertEqual(path.read_bytes(), before)
        self.assertFalse(list(path.parent.glob(".nr-json-*")))

    def test_reset_one_and_all_write_defaults_consumed_by_actual_live_settings_parser(self):
        # Execute just the real daemon's Settings class, avoiding its GPU imports.
        daemon_source = (Path(core.__file__).resolve().parent.parent / "layer/nr_daemon.py").read_text(encoding="utf-8")
        tree = ast.parse(daemon_source)
        actual_class = next(node for node in tree.body if isinstance(node, ast.ClassDef) and node.name == "Settings")
        namespace = {"os": os, "math": math, "json": json,
                     "nr_frame": SimpleNamespace(PROFILES={choice: {} for choice in ("standard", "natural", "cinematic", "neutral")})}
        exec(compile(ast.Module(body=[actual_class], type_ignores=[]), "nr_daemon.py", "exec"), namespace)
        path = self.root / "work/nr_settings.json"
        result = core.save_settings(self.root, {"render_scale": 0.25, "intensity": 0.3, "profile": "natural"})
        values = core._read_json(path)
        values["other_tool"] = 7
        core._atomic_json(path, values)
        args = SimpleNamespace(settings=str(path), **result["defaults"])
        live = namespace["Settings"](args)
        live.refresh()
        self.assertEqual((live.render_scale, live.intensity, live.profile), (0.25, 0.3, "natural"))
        result = core.reset_settings(self.root, ["render_scale"])
        live.stamp = None  # Force observation even on a coarse-mtime test filesystem.
        live.refresh()
        self.assertEqual((live.render_scale, live.intensity, live.profile), (1, 0.3, "natural"))
        self.assertEqual(core._read_json(path)["other_tool"], 7)
        result = core.reset_settings(self.root)
        live.stamp = None
        live.refresh()
        self.assertEqual({name: getattr(live, name) for name in live.KNOBS}, result["defaults"])
        self.assertEqual(set(result["explicit_keys"]), set(result["defaults"]))
        self.assertEqual(core._read_json(path)["other_tool"], 7)

    def test_unknown_reset_does_not_change_settings_or_trigger(self):
        path = self.root / "work/nr_settings.json"
        core._atomic_json(path, {"render_scale": 0.4})
        before = path.read_bytes()
        with self.assertRaises(ValueError):
            core.reset_settings(self.root, ["master", "render_scale"])
        self.assertEqual(path.read_bytes(), before)
        self.assertFalse((self.root / "work/nr_trigger").exists())

    def test_non_regular_settings_destination_is_rejected(self):
        path = self.root / "work/nr_settings.json"
        path.mkdir()
        sentinel = path / "foreign.txt"
        sentinel.write_bytes(b"preserve")
        with self.assertRaises(ValueError):
            core.save_settings(self.root, {"render_scale": 0.3})
        self.assertEqual(sentinel.read_bytes(), b"preserve")

    def test_staging_failure_preserves_existing_owned_installation(self):
        destination = self.own()
        sentinel = destination / "old.dll"
        sentinel.write_bytes(b"previous installation")
        with patch.object(core, "_copy_runtime", side_effect=OSError("fixture copy failed")):
            result = core.install(self.profile)
        self.assertFalse(result["ok"])
        self.assertEqual(sentinel.read_bytes(), b"previous installation")
        self.assertFalse(list(self.game.parent.glob(".dlss-nr-stage-*")))

    def test_late_commit_failure_rolls_back_installation_settings_profile_and_trigger(self):
        destination = self.own()
        sentinel = destination / "old.dll"
        sentinel.write_bytes(b"previous installation")
        core.save_profile(self.profile)
        core._atomic_json(self.root / "work/nr_settings.json", {"render_scale": 0.9})
        (self.root / "work/nr_trigger").write_bytes(b"old trigger")
        paths = [core.profile_path(self.root), self.root / "work/nr_settings.json", self.root / "work/nr_trigger"]
        before = {path: path.read_bytes() for path in paths}
        with patch.object(core, "save_profile", side_effect=ValueError("fixture profile failure")):
            result = core.install(self.profile)
        self.assertFalse(result["ok"])
        self.assertEqual(sentinel.read_bytes(), b"previous installation")
        self.assertEqual({path: path.read_bytes() for path in paths}, before)
        self.assertFalse(list(self.game.parent.glob(".dlss-nr-backup-*")))

    def test_extraction_failure_never_writes_game_even_with_success_exit_code(self):
        self.weights.unlink()
        dll = write_pe(self.base / "owned DLL (fixture)" / "nvngx_dlssnr.dll")
        profile = replace(self.profile, dll=dll)
        self.assertIn("version unknown; tested with 310.8.0.0", self.checks(profile)["local_model_input"]["detail"])
        with patch.object(core, "_run", return_value={"returncode": 0, "timed_out": False,
                         "output": "Done without usable weights"}) as run:
            result = core.install(profile)
        self.assertFalse(result["ok"])
        self.assertIn("version unknown (tested: 310.8.0.0)", result["error"])
        self.assertEqual(run.call_args.args[0][3], dll)
        self.assertFalse(core.installed_path(profile).exists())
        # What the extractor said outlives the window's next action, for Save report.
        self.assertEqual((self.root / core.WEIGHTS_LOG).read_text(encoding="utf-8"), "Done without usable weights")

    def test_missing_packages_only_uses_prepared_local_interpreter_consistently(self):
        local = write_pe(self.root / "work/windows-python/Scripts/python.exe")
        chosen = replace(self.profile, python=local)
        self.native.side_effect = lambda python, **kwargs: python_info() if Path(python) == local else python_info(packages={}, package_errors={"numpy": "missing", "safetensors": "missing"})
        with patch.object(core, "ensure_dependencies", return_value={"ok": True, "profile": chosen.to_dict(), "python": str(local)}) as prepare:
            result = core.install(self.profile)
        self.assertTrue(result["ok"], result)
        prepare.assert_called_once()
        self.assertEqual(core.load_profile(self.root).python, local)
        self.assertEqual(core.runtime_env(core.Profile.from_dict(result["profile"]))["NR_PYTHON"], str(local))

    def test_prepare_python_venv_and_pip_are_local_argv_without_shell(self):
        calls = []
        def run(command, **kwargs):
            calls.append([str(arg) for arg in command])
            if "venv" in command:
                directory = Path(command[-1])
                write_pe(directory / "Scripts/python.exe")
                (directory / "pyvenv.cfg").write_text("home = fixture\n")
            return {"returncode": 0, "timed_out": False, "output": "fixture"}
        with patch.object(core, "_run", side_effect=run):
            result = core.ensure_dependencies(self.profile)
        self.assertTrue(result["ok"], result)
        local = self.root / "work/windows-python/Scripts/python.exe"
        self.assertEqual(calls[0][:4], [str(self.python), "-I", "-m", "venv"])
        self.assertEqual(calls[1][:4], [str(local), "-I", "-m", "pip"])
        self.assertIn("--only-binary=:all:", calls[1])
        self.assertEqual(result["profile"]["python"], str(local))

    def test_foreign_local_python_directory_is_preserved(self):
        directory = self.root / "work/windows-python"
        directory.mkdir()
        sentinel = directory / "foreign.txt"
        sentinel.write_bytes(b"preserve")
        with patch.object(core, "_run") as run:
            self.assertFalse(core.ensure_dependencies(self.profile)["ok"])
        run.assert_not_called()
        self.assertEqual(sentinel.read_bytes(), b"preserve")

    def test_install_nr_returns_steams_launch_options_first(self):
        # An earlier setup could leave Steam starting a game through NR's wrapper. That no
        # longer holds the profile; Install NR returns those options before it installs.
        import windows_launch as launch
        import windows_wizard as bridge
        core.save_profile(self.profile)
        core._atomic_json(self.root / "work/windows-wizard/steam-backup.json", {"restored": False})
        other = replace(self.profile, game_exe=write_pe(self.base / "other game" / "other.exe"))
        core.save_profile(other)
        self.assertEqual(core.load_profile(self.root), other)
        with patch.object(launch, "restore_steam", return_value={"ok": False, "error": "A Steam game is running"}), \
                patch.object(core, "install") as install:
            self.assertEqual(bridge.install_nr(other, None)["error"], "A Steam game is running")
        install.assert_not_called()
        with patch.object(launch, "restore_steam", return_value={"ok": True, "changed": True}) as restore:
            result = bridge.install_nr(other, None)
        self.assertTrue(result["ok"], result)
        self.assertTrue(result["steam_restored"])
        self.assertEqual(restore.call_args.args[0], other)
        self.assertTrue(core.installed_path(other).exists())

    def test_install_blocks_active_selected_game_and_other_root_session(self):
        self.launch_state()
        self.assertFalse(core.install(self.profile)["ok"])
        other = replace(self.profile, game_exe=write_pe(self.base / "other game" / "other.exe"))
        self.assertFalse(core.install(other)["ok"])
        self.assertFalse(core.installed_path(other).exists())

    def test_install_rechecks_game_after_extraction_and_before_commit(self):
        self.processes.side_effect = [[], [], [{"ProcessId": 12, "ExecutablePath": str(self.game)}]]
        self.assertFalse(core.install(self.profile)["ok"])
        self.assertFalse(core.installed_path(self.profile).exists())
        self.assertFalse(list(self.game.parent.glob(".dlss-nr-stage-*")))

    def test_owned_trigger_off_on_does_not_claim_processing(self):
        self.own()
        self.assertTrue(core.set_effect(self.profile, False)["ok"])
        enabled = core.set_effect(self.profile, True)
        self.assertTrue(enabled["ok"])
        self.assertFalse(enabled["processing_confirmed"])
        self.assertTrue((self.root / "work/nr_trigger").is_file())
        self.assertTrue(core.set_effect(self.profile, False)["ok"])
        self.assertFalse((self.root / "work/nr_trigger").exists())

    def test_directory_trigger_returns_failure_without_exception(self):
        self.own()
        (self.root / "work/nr_trigger").mkdir()
        result = core.set_effect(self.profile, True)
        self.assertFalse(result["ok"])
        self.assertTrue((self.root / "work/nr_trigger").is_dir())

    def test_broken_link_trigger_cannot_write_outside_release(self):
        self.own()
        outside = self.base / "outside-trigger"
        trigger = self.root / "work/nr_trigger"
        try:
            trigger.symlink_to(outside)
        except OSError as error:
            self.skipTest("Creating symlinks is unavailable: " + str(error))
        self.assertFalse(core.set_effect(self.profile, True)["ok"])
        self.assertFalse(outside.exists())

    def test_other_profile_cannot_toggle_active_root_game(self):
        self.launch_state()
        other = replace(self.profile, game_exe=write_pe(self.base / "other game" / "other.exe"))
        self.own(other)
        trigger = self.root / "work/nr_trigger"
        trigger.write_bytes(b"active A")
        self.assertFalse(core.set_effect(other, False)["ok"])
        self.assertEqual(trigger.read_bytes(), b"active A")

    def test_status_persists_between_processes_and_excludes_previous_launch(self):
        log = self.root / "work/nr_daemon.log"
        historical = b"half rounding: nearest\nmodel ready in 1s\nkeeping NumPy's large blocks\n800x450 rgba in 1.0s change 0.1 network 320x320\n"
        log.write_bytes(historical)
        self.launch_state(offset=len(historical))
        first = core.status(self.profile)
        self.assertEqual(first["processed"], 0)
        self.assertIsNone(first["half_probe"])
        self.assertFalse(first["allocator"])
        self.assertFalse(first["fresh_frames"])
        core._LOG_STATES.clear()  # each GUI poll starts a separate Python process
        with log.open("ab") as handle:
            handle.write(b"half rounding: nearest\nmodel ready in 1s\nkeeping NumPy's large blocks\n800x450 rgba in 0.2s change 0.1 network 320x320\nframe rejected/failed bad format\n")
        second = core.status(self.profile)
        self.assertEqual(second["processed"], 1)
        self.assertEqual(second["fresh_processed"], 1)
        self.assertTrue(second["fresh_frames"])
        self.assertEqual(second["network_shape"], [320, 320])
        self.assertTrue(second["half_probe"]["ok"])
        self.assertTrue(second["allocator"])
        self.assertEqual(second["rejected"], 1)
        core._LOG_STATES.clear()
        third = core.status(self.profile)
        self.assertEqual(third["processed"], 1)
        self.assertEqual(third["fresh_processed"], 0)
        self.assertFalse(third["fresh_frames"])
        self.launch_state(pid=124, offset=log.stat().st_size, started="new session")
        core._LOG_STATES.clear()
        fourth = core.status(self.profile)
        self.assertEqual(fourth["processed"], 0)
        self.assertIsNone(fourth["half_probe"])
        self.assertFalse(fourth["allocator"])

    def test_historical_daemon_log_does_not_confirm_game_connection(self):
        (self.root / "work/nr_daemon.log").write_text("800x450 rgba in 0.2s change 0.1 network 320x320\n", encoding="utf-8")
        result = core.status(self.profile)
        self.assertEqual(result["processed"], 1)
        self.assertIsNone(result["game_connected"])
        self.assertFalse(result["fresh_frames"])
        self.assertFalse(result["active_game"])

    def test_status_partial_lines_survive_process_restart(self):
        log = self.root / "work/nr_daemon.log"
        log.write_bytes(b"800x450 rgba in 0.2s change")
        self.launch_state()
        self.assertEqual(core.status(self.profile)["processed"], 0)
        core._LOG_STATES.clear()
        with log.open("ab") as handle:
            handle.write(b" 0.1 network 320x320\n")
        result = core.status(self.profile)
        self.assertEqual(result["processed"], 1)
        self.assertTrue(result["fresh_frames"])

    def test_status_read_is_bounded_and_labels_counts_incomplete(self):
        log = self.root / "work/nr_daemon.log"
        log.write_bytes(b"uninteresting\n" * (core.MAX_LOG_READ // 10) + b"800x450 rgba in 0.2s change 0.1 network 320x320\n")
        result = core.status(self.profile)
        self.assertFalse(result["counts_complete"])
        self.assertEqual(result["processed"], 1)

    def test_status_preserves_failed_probe_and_ignores_malformed_timing(self):
        (self.root / "work/nr_daemon.log").write_text("half rounding: packHalf2x16 3/1024\nFAIL: rounding mismatch\nmodel ready in 1s\n800x450 rgba in 1..2s change 0.1\n")
        result = core.status(self.profile)
        self.assertFalse(result["half_probe"]["ok"])
        self.assertEqual(result["processed"], 0)
        self.assertEqual(result["errors"], 1)

    def test_export_without_git_or_settings_includes_diagnostics_only(self):
        core._atomic_json(self.root / "release-metadata.json", {"schema_version": 1, "source_commit": "04459d2", "source_branch": "improve-release"})
        (self.root / "work/nr_daemon.log").write_text("fixture daemon output\n")
        report = self.base / "reports (local)" / "result.json"
        with patch.object(core, "_system_metadata", return_value={"gpu": [{"Name": "fixture GPU", "DriverVersion": "fixture"}]}), patch.object(core, "_run", side_effect=AssertionError("No Git or executable needed")):
            result = core.export_report(self.profile, report)
        self.assertTrue(result["ok"], result)
        saved = core._read_json(report)
        self.assertIsNone(saved["settings"])
        self.assertTrue(saved["settings_error"])
        self.assertEqual(saved["release"]["source_commit"], "04459d2")
        self.assertIn("fixture daemon output", saved["daemon_log_tail"])
        # What Check says, so a report of a failed installation names the item (issue #12).
        self.assertIn("runtime_dependencies", {item["name"] for item in saved["checks"]})
        self.assertEqual(saved["dll"], {"present": False})
        self.assertEqual(saved["weights_log_tail"], "")
        self.assertNotIn("fixture_648", report.read_text(encoding="utf-8"))
        self.assertFalse(core.export_report(self.profile, self.base / "report.dll")["ok"])

    def test_a_python_or_a_library_that_fails_says_why_in_its_check(self):
        self.native.return_value = python_info(probe_ok=False, probe_error="no answer in 30 s")
        self.assertIn("did not answer: no answer in 30 s", self.checks()["python_native_x64"]["detail"])
        self.native.return_value = python_info()
        self.deps.return_value = {"ok": False, "failures": {
            str(self.root / "work/libnr_image.dll"): "Could not find module",
            "VCOMP140.DLL": "missing: install the Microsoft Visual C++ x64 Redistributable"}}
        detail = self.checks()["runtime_dependencies"]["detail"]
        self.assertIn("libnr_image.dll: Could not find module", detail)
        self.assertIn("VCOMP140.DLL: missing: install the Microsoft Visual C++ x64 Redistributable", detail)


class ProbeTests(unittest.TestCase):
    """What the Python and library probes say when they fail; the probe's process stood in for."""

    def answering(self, **got):
        result = {"command": [], "returncode": 0, "stdout": "", "stderr": "", "output": "", "timed_out": False}
        result.update(got)
        return patch.object(core, "_run", return_value=result)

    def test_a_missing_openmp_runtime_is_named_with_its_installer(self):
        profile = SimpleNamespace(python=Path("python.exe"), root=Path("release"))
        answer = json.dumps({"loaded": ["vulkan-1.dll"], "failures": {
            "release/work/libnr_image.dll": "Could not find module", "VCOMP140.DLL": "missing"}})
        with self.answering(stdout=answer):
            result = core._load_dependencies(profile)
        self.assertFalse(result["ok"])
        self.assertIn("Visual C++ x64 Redistributable", result["failures"]["VCOMP140.DLL"])
        self.assertIn(core.VC_REDIST, result["failures"]["VCOMP140.DLL"])

    @unittest.skipUnless(os.name == "nt", "Windows' version resource")
    def test_a_files_version_is_read_from_its_resource(self):
        kernel = Path(os.environ["SystemRoot"]) / "System32" / "kernel32.dll"
        self.assertRegex(core._file_version(kernel) or "", r"^\d+\.\d+\.\d+\.\d+$")
        with tempfile.TemporaryDirectory() as folder:
            self.assertIsNone(core._file_version(write_pe(Path(folder) / "no version.dll")))
            self.assertIsNone(core._file_version(Path(folder) / "absent.dll"))

    def test_probes_that_do_not_answer_say_so(self):
        profile = SimpleNamespace(python=Path("python.exe"), root=Path("release"))
        with self.answering(returncode=None, timed_out=True):
            self.assertEqual(core._load_dependencies(profile)["failures"], {"dependency_probe": "no answer in 30 s"})
            info = core._python_info(Path("python.exe"))
        self.assertFalse(info["probe_ok"])
        self.assertEqual(info["probe_error"], "no answer in 30 s")


class CommandTests(unittest.TestCase):
    def test_capture_both_streams_exact_argv_and_exit_code(self):
        result = core._run([sys.executable, "-I", "-c", "import sys; print(sys.argv[1]); print('stderr fixture',file=sys.stderr); sys.exit(7)", "spaces (and parentheses)"], timeout=5)
        self.assertEqual(result["returncode"], 7)
        self.assertIn("spaces (and parentheses)", result["stdout"])
        self.assertIn("stderr fixture", result["stderr"])

    def test_deadline_applies_when_exited_parent_descendant_keeps_output_open(self):
        # Child is ours, CPU-only and self-terminates; no game or daemon is touched.
        child = "import sys,time; end=time.monotonic()+3;\nwhile time.monotonic()<end: print('x'*100,flush=True); time.sleep(.001)"
        parent = "import subprocess,sys; subprocess.Popen([sys.executable,'-I','-c',sys.argv[1]])"
        started = time.monotonic()
        result = core._run([sys.executable, "-I", "-c", parent, child], timeout=0.1)
        self.assertTrue(result["timed_out"])
        self.assertLess(time.monotonic() - started, 2.8)
        time.sleep(1)  # the finite child finishes before this test process exits


@unittest.skipUnless(os.name == "nt", "asks Windows about a process")
class RunningExecutableTests(unittest.TestCase):
    """The status check's one query: a live process by its path, a finished one not at all."""

    def test_live_process_reports_its_executable(self):
        # Not sys.executable: a virtual environment's python.exe starts the base interpreter.
        import ctypes
        own = ctypes.create_unicode_buffer(32768)
        ctypes.windll.kernel32.GetModuleFileNameW(None, own, len(own))
        self.assertEqual(core._canonical(core._running_executable(os.getpid())),
                         core._canonical(own.value))

    def test_finished_and_invalid_processes_report_nothing(self):
        child = subprocess.Popen([sys.executable, "-c", "pass"])
        child.wait(timeout=60)
        self.assertIsNone(core._running_executable(child.pid))
        for pid in (0, -1, None, True, "12"):
            self.assertIsNone(core._running_executable(pid))


if __name__ == "__main__":
    unittest.main()
