"""CPU-only launch/Steam regressions. No Steam, game, or GPU is started."""
from __future__ import annotations

from dataclasses import replace
import json
import os
from pathlib import Path
import subprocess
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

import windows_launch as launch
from windows_wizard_core import Profile, pe_architecture


def localconfig(options="+set test 1", app_id="379720", newline="\r\n"):
    option = '' if options is None else '\t"LaunchOptions" ' + launch._vdf_quote(options)
    return ('// retain this comment\n"UserLocalConfigStore" { "Software" { "Valve" { "Steam" {\n'
            '"Unrelated" "a\\\\b"\n"apps" {\n"' + app_id + '" {\n'
            '"LastPlayed" "123456"\n' + option + '\n}\n'
            '"999" { "Other" "unchanged" }\n} } } } }\n').replace('\n', newline).encode('utf-8')


class LaunchTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="nr-launch-tests-")
        self.addCleanup(self.temporary.cleanup)
        # Resolved, as the launcher resolves what it is given: a TEMP in 8.3 form
        # (C:\Users\NAME~1\...) otherwise compares unequal to the same file's long name.
        self.base = Path(self.temporary.name).resolve()
        self.root = self.base / "package (候補) & test"
        self.game = self.base / "library (games)" / "steamapps/common/DOOM"
        self.game.mkdir(parents=True)
        self.exe = self.game / "DOOMx64vk.exe"
        self.default_exe = self.game / "DOOMx64.exe"
        self.python = self.base / "Python (custom) & 日本語/python.exe"
        self.python.parent.mkdir(parents=True)
        self.python.write_bytes(b"test interpreter")
        self.exe.write_bytes(b"selected x64 game")
        self.default_exe.write_bytes(b"Steam default x64 game")
        self.dll = self.base / "my dll (own)/nvngx_dlssnr.dll"
        self.dll.parent.mkdir(parents=True)
        self.dll.write_bytes(b"own DLL")
        scripts = self.root / "scripts"
        scripts.mkdir(parents=True)
        (scripts / "windows_launch.py").write_text("# packaged wrapper", encoding="utf-8")
        self.profile = Profile(self.root, self.python, self.exe, self.dll,
                               game_args=["+set", "r_width", "800", "literal & $(text)"])
        self._install_marker(self.profile)
        self.trigger = self.root / "work/nr_trigger"
        self.trigger.parent.mkdir(parents=True, exist_ok=True)
        self.trigger.write_bytes(b"on")
        self.daemon = self.root / "work/nr_daemon.log"
        self.daemon.write_bytes(b"previous session\n")
        self.core = SimpleNamespace(
            SCHEMA_VERSION=1, Profile=Profile, pe_architecture=pe_architecture,
            validate=Mock(return_value={"ok": True}),
            runtime_env=Mock(side_effect=self._environment),
            set_effect=Mock(side_effect=self._effect),
            save_profile=Mock(side_effect=AssertionError("Steam must not overwrite GUI profile")),
            profile_path=lambda root: Path(root) / "work/windows-profile.json")
        self.addCleanup(patch.stopall)
        patch.object(launch, "_core", return_value=self.core).start()
        self.processes = patch.object(launch, "_processes", return_value=[]).start()
        patch.object(launch, "_registry", return_value=None).start()
        self.process = Mock(pid=456)
        self.process.poll.return_value = None
        self.process.wait.return_value = 0
        self.popen = patch.object(launch.subprocess, "Popen", return_value=self.process).start()
        patch.object(launch.subprocess, "run", side_effect=AssertionError("No real commands in tests")).start()
        self.stop = patch.object(launch, "_stop_steam", return_value=False).start()
        self.restart = patch.object(launch, "_restart_steam", return_value=789).start()
        self.steam = self.base / "Steam (client)/steam.exe"
        self.steam.parent.mkdir(parents=True)
        self.steam.write_bytes(b"fake steam")
        self.config = self.steam.parent / "userdata/123/config/localconfig.vdf"
        self.config.parent.mkdir(parents=True)
        self.config.write_bytes(localconfig())
        self.manifest = self.game.parent.parent / "appmanifest_379720.acf"
        self.manifest.write_text('"AppState" { "appid" "379720" "installdir" "DOOM" }',
                                 encoding="utf-8")

    def _install_marker(self, profile):
        directory = profile.game_exe.parent / "dlss-nr"
        directory.mkdir(parents=True, exist_ok=True)
        (directory / ".windows-wizard-owner.json").write_text(
            json.dumps({"schema_version": 1, "root": str(profile.root)}), encoding="utf-8")

    def _environment(self, profile):
        return {"NR_ROOT": str(profile.root), "NR_PYTHON": str(profile.python),
                "NR_LAYER_SOCKET": r"\\.\pipe\nr_windows_fixture",
                "NR_LAYER_TRIGGER": str(self.trigger), "NR_LAYER_LOG": str(self.daemon),
                "VK_LAYER_PATH": str(profile.game_exe.parent / "dlss-nr"),
                "ENABLE_NR_LAYER": "1", "NORMAL": "retained"}

    def _effect(self, profile, enabled):
        self.assertFalse(enabled)
        self.trigger.unlink(missing_ok=True)
        return {"ok": True}

    def _configure(self, profile=None):
        return launch.configure_steam(profile or self.profile, self.steam)

    def test_direct_argv_env_logs_and_fresh_effect_off_without_git(self):
        def started(command, **kwargs):
            kwargs["stdout"].write(b"game stdout\n")
            kwargs["stderr"].write(b"game stderr\n")
            return self.process
        self.popen.side_effect = started
        result = launch.launch(self.profile)
        self.assertTrue(result["ok"], result)
        command, kwargs = self.popen.call_args
        self.assertEqual(command[0], [str(self.exe), *self.profile.game_args])
        self.assertFalse(kwargs["shell"])
        self.assertEqual(kwargs["cwd"], str(self.game))
        self.assertEqual(kwargs["env"]["NR_PYTHON"], str(self.python))
        self.assertEqual(kwargs["env"]["NORMAL"], "retained")
        self.assertEqual(result["pid"], 456)
        self.assertEqual(result["daemon_start_bytes"], len(b"previous session\n"))
        self.assertEqual(Path(result["stdout_log"]).read_bytes(), b"game stdout\n")
        self.assertEqual(Path(result["stderr_log"]).read_bytes(), b"game stderr\n")
        self.assertFalse(self.trigger.exists())
        self.assertFalse((self.root / ".git").exists())

    def test_running_selected_game_preserves_trigger_without_launch(self):
        self.processes.return_value = [{"Name": self.exe.name, "ProcessId": 100,
                                      "ExecutablePath": str(self.exe)}]
        result = launch.launch(self.profile)
        self.assertFalse(result["ok"])
        self.popen.assert_not_called()
        self.core.set_effect.assert_not_called()
        self.assertEqual(self.trigger.read_bytes(), b"on")

    def test_running_previous_game_sharing_root_preserves_state_and_trigger(self):
        other = self.base / "another game (B)/game.exe"
        other.parent.mkdir()
        other.write_bytes(b"another x64 game")
        profile_b = replace(self.profile, game_exe=other)
        self._install_marker(profile_b)
        state_path = self.root / "work/windows-wizard/launch-state.json"
        state_path.parent.mkdir()
        state = {"root": str(self.root), "game_exe": str(self.exe), "pid": 100}
        initial = json.dumps(state).encode("utf-8")
        state_path.write_bytes(initial)
        self.processes.return_value = [{"Name": self.exe.name, "ProcessId": 101,
                                      "ExecutablePath": str(self.exe)}]
        result = launch.launch(profile_b)
        self.assertFalse(result["ok"])
        self.assertEqual(state_path.read_bytes(), initial)
        self.assertEqual(self.trigger.read_bytes(), b"on")
        self.core.set_effect.assert_not_called()
        self.popen.assert_not_called()

    def test_uninspectable_selected_game_and_wrong_owner_are_refused(self):
        self.processes.return_value = [{"Name": self.exe.name, "ProcessId": 100,
                                      "ExecutablePath": None}]
        self.assertFalse(launch.launch(self.profile)["ok"])
        self.processes.return_value = []
        owner = self.game / "dlss-nr/.windows-wizard-owner.json"
        owner.write_text(json.dumps({"schema_version": 1, "root": str(self.base / "foreign")}),
                         encoding="utf-8")
        self.assertFalse(launch.launch(self.profile)["ok"])
        self.core.set_effect.assert_not_called()
        self.popen.assert_not_called()

    def test_runtime_interpreter_mismatch_never_launches(self):
        environment = self._environment(self.profile)
        environment["NR_PYTHON"] = "another-python.exe"
        self.core.runtime_env.side_effect = None
        self.core.runtime_env.return_value = environment
        self.assertFalse(launch.launch(self.profile)["ok"])
        self.core.set_effect.assert_not_called()
        self.popen.assert_not_called()

    def test_steam_default_executable_substitution_preserves_command_tail(self):
        result, _ = launch._launch_direct(
            self.profile, original_command=[str(self.default_exe), "+set", "old_option", "two words"])
        self.assertTrue(result["executable_substituted"])
        self.assertEqual(result["command"], [str(self.exe), "+set", "old_option", "two words",
                                             *self.profile.game_args])
        self.assertEqual(result["steam_original_exe"], str(self.default_exe))

    def test_foreign_original_executable_is_not_executed(self):
        foreign = self.base / "custom wrapper.exe"
        foreign.write_bytes(b"wrapper")
        with self.assertRaises(launch.LaunchError):
            launch._launch_direct(self.profile, original_command=[str(foreign)])
        self.popen.assert_not_called()
        self.core.set_effect.assert_not_called()

    def test_immediate_failure_is_reported_with_logs_and_exit_code(self):
        self.process.poll.return_value = 5
        result = launch.launch(self.profile)
        self.assertFalse(result["ok"])
        self.assertEqual(result["exit_code"], 5)
        self.assertTrue(Path(result["stderr_log"]).is_file())

    def test_quoting_round_trip_for_non_ascii_shell_characters_and_backslashes(self):
        args = [str(self.python), str(self.root / "scripts/windows_launch.py"), "a & $(b); %HOME%",
                "D:\\end\\", 'embedded "quote"', "", "two words", "日本語 (test)"]
        command = " ".join(launch._quote(arg) for arg in args)
        self.assertEqual(launch._split_commandline(command), args)

    def test_wrapper_snapshot_keeps_exact_selected_python(self):
        path = launch._ensure_steam_profile(self.profile)
        parsed = launch._split_commandline(launch.steam_launch_options(self.profile))
        self.assertEqual(parsed, [str(self.python), str(self.root / "scripts/windows_launch.py"),
                                  "--steam-wrapper", str(path), "--", "%command%"])
        self.assertEqual(json.loads(path.read_text(encoding="utf-8")), self.profile.to_dict())
        draft = self.root / "work/windows-profile.json"
        draft.write_text('{"draft": "unrelated"}', encoding="utf-8")
        self.assertEqual(json.loads(path.read_text(encoding="utf-8"))["python"], str(self.python))

    def _write_pe(self, path, machine=0x8664, magic=0x20b):
        data = bytearray(128)
        data[:2] = b"MZ"
        struct.pack_into("<I", data, 60, 64)
        data[64:68] = b"PE\0\0"
        struct.pack_into("<H", data, 68, machine)
        struct.pack_into("<H", data, 88, magic)
        path.write_bytes(data)

    def test_x64_pythonw_hides_only_wrapper_and_keeps_snapshot_and_runtime_python(self):
        snapshot = launch._ensure_steam_profile(self.profile)
        before = snapshot.read_bytes()
        profile_before = self.profile.to_dict()
        pythonw = self.python.with_name("pythonw.exe")
        self._write_pe(pythonw)
        options = launch._split_commandline(launch.steam_launch_options(self.profile))
        self.assertEqual(options[0], str(pythonw))
        self.assertEqual(options[3], str(snapshot))
        self.assertEqual(self.profile.to_dict(), profile_before)
        self.assertEqual(snapshot.read_bytes(), before)
        self.assertEqual(self._environment(self.profile)["NR_PYTHON"], str(self.python))
        result, _ = launch._launch_direct(self.profile, original_command=[str(self.default_exe)])
        self.assertTrue(result["ok"], result)
        self.assertEqual(self.popen.call_args.kwargs["env"]["NR_PYTHON"], str(self.python))

    def test_missing_pythonw_uses_selected_python(self):
        self.assertFalse(self.python.with_name("pythonw.exe").exists())
        self.assertEqual(launch._split_commandline(launch.steam_launch_options(self.profile))[0],
                         str(self.python))

    def test_bad_pe_or_x86_pythonw_uses_selected_python(self):
        pythonw = self.python.with_name("pythonw.exe")
        for content in (b"not PE", b"MZ"):
            with self.subTest(content=content):
                pythonw.write_bytes(content)
                self.assertEqual(launch._steam_python(self.profile), self.python)
        self._write_pe(pythonw, machine=0x14c, magic=0x10b)
        self.assertEqual(launch._steam_python(self.profile), self.python)
        self._write_pe(pythonw, machine=0x8664, magic=0x10b)
        self.assertEqual(launch._steam_python(self.profile), self.python)

    def test_symlink_or_directory_pythonw_uses_selected_python(self):
        pythonw = self.python.with_name("pythonw.exe")
        self._write_pe(pythonw)
        with patch.object(Path, "is_symlink", autospec=True,
                          side_effect=lambda path: path == pythonw):
            self.assertEqual(launch._steam_python(self.profile), self.python)
        pythonw.unlink()
        pythonw.mkdir()
        self.assertEqual(launch._steam_python(self.profile), self.python)

    def test_wrapper_error_with_no_stderr_still_records_json(self):
        snapshot = launch._ensure_steam_profile(self.profile)
        self.core.validate.return_value = {"ok": False, "error": "missing runtime"}
        with patch.object(launch.sys, "stderr", None), patch("builtins.print") as printed:
            code = launch.main(["--steam-wrapper", str(snapshot), "--", str(self.default_exe)])
        self.assertEqual(code, 1)
        printed.assert_not_called()
        failure = self.root / "work/windows-wizard/steam-wrapper-last-error.json"
        recorded = json.loads(failure.read_text(encoding="utf-8"))
        self.assertFalse(recorded["ok"])
        self.assertEqual(recorded["error"], "missing runtime")
        self.assertEqual(recorded["profile_snapshot"], str(snapshot))
        self.popen.assert_not_called()

    def test_modified_snapshot_is_not_overwritten(self):
        path = launch._ensure_steam_profile(self.profile)
        path.write_text('{"foreign": true}', encoding="utf-8")
        result = self._configure()
        self.assertFalse(result["ok"])
        self.assertIsNone(result["launch_options"])
        self.assertEqual(path.read_text(encoding="utf-8"), '{"foreign": true}')
        self.stop.assert_not_called()

    def test_app_id_detection_in_nested_install_with_unrelated_bad_manifest(self):
        nested = self.game / "bin/x64/game.exe"
        nested.parent.mkdir(parents=True)
        nested.write_bytes(b"game")
        (self.manifest.parent / "appmanifest_bad.acf").write_text('"broken" {', encoding="utf-8")
        self.assertEqual(launch.detect_steam_app_id(nested), "379720")
        (self.manifest.parent / "appmanifest_duplicate.acf").write_text(
            '"AppState" { "appid" "99" "installdir" "DOOM" }', encoding="utf-8")
        self.assertIsNone(launch.detect_steam_app_id(nested))
        self.assertIsNone(launch.detect_steam_app_id(self.python))

    def test_vdf_edit_escaping_preserves_other_bytes_bom_and_line_endings(self):
        before = b"\xef\xbb\xbf" + localconfig('+set path "D:\\game (test)\\data"')
        value = '"D:\\Python (test)\\python.exe" "D:\\候補 & test\\wrapper.py" -- %command%'
        after = launch._edit_options(before, "379720", value)
        self.assertEqual(launch._options(after, "379720"), value)
        self.assertTrue(after.startswith(b"\xef\xbb\xbf"))
        self.assertIn(b'// retain this comment\r\n', after)
        self.assertIn(b'"999" { "Other" "unchanged" }', after)
        self.assertEqual(launch._edit_options(after, "379720", launch._options(before, "379720")), before)

    def test_vdf_absent_option_restore_and_new_app(self):
        before = localconfig(None)
        after = launch._edit_options(before, "379720", "wrapper")
        restored = launch._edit_options(after, "379720", None)
        self.assertIsNone(launch._options(restored, "379720"))
        self.assertIn(b'"LastPlayed" "123456"', restored)
        new_app = launch._edit_options(before, "101", "other wrapper")
        self.assertEqual(launch._options(new_app, "101"), "other wrapper")
        self.assertEqual(launch._options(new_app, "379720"), None)

    def test_duplicate_vdf_key_refuses_ambiguous_options(self):
        data = localconfig().replace(b'"LastPlayed" "123456"', b'"LaunchOptions" "another"')
        with self.assertRaises(launch.LaunchError):
            launch._edit_options(data, "379720", "replacement")

    def test_atomic_edit_conflicts_keep_concurrent_file(self):
        before = self.config.read_bytes()
        self.config.write_bytes(b"foreign update")
        with self.assertRaises(launch.LaunchError):
            launch._atomic_write(self.config, before, b"replacement")
        self.assertEqual(self.config.read_bytes(), b"foreign update")
        self.config.write_bytes(before)
        with patch.object(launch.os, "fsync", side_effect=lambda _: self.config.write_bytes(b"second update")):
            with self.assertRaises(launch.LaunchError):
                launch._atomic_write(self.config, before, b"replacement")
        self.assertEqual(self.config.read_bytes(), b"second update")
        self.assertEqual(list(self.config.parent.glob("*.nr-*.tmp")), [])

    def test_configure_and_restore_preserve_existing_game_args_and_fields(self):
        before = self.config.read_bytes()
        original = launch._options(before, "379720")
        result = self._configure()
        self.assertTrue(result["ok"], result)
        self.assertEqual(result["launch_options"], launch.steam_launch_options(self.profile) + " " + original)
        self.assertIn(b'"999" { "Other" "unchanged" }', self.config.read_bytes())
        backup_path = Path(result["backup"])
        backup = json.loads(backup_path.read_text(encoding="utf-8"))
        self.assertEqual(backup["original_launch_options"], original)
        self.assertEqual(backup["profile_snapshot"], str(launch._steam_profile_path(self.profile)))
        again = self._configure()
        self.assertTrue(again["already_configured"])
        self.assertEqual(self.stop.call_count, 1)
        restored = launch.restore_steam(self.profile)
        self.assertTrue(restored["ok"], restored)
        self.assertEqual(self.config.read_bytes(), before)
        self.assertTrue(json.loads(backup_path.read_text(encoding="utf-8"))["restored"])
        self.core.save_profile.assert_not_called()

    def test_foreign_wrappers_and_shell_options_are_preserved(self):
        for options in ['"custom python.exe" wrapper.py %command%', 'cmd /c start game.exe',
                        '+set test 1 & other.exe', '+set "unterminated']:
            with self.subTest(options=options):
                before = localconfig(options)
                self.config.write_bytes(before)
                result = self._configure()
                self.assertFalse(result["ok"])
                self.assertTrue(result["manual"])
                self.assertEqual(self.config.read_bytes(), before)
                self.assertTrue(Path(launch._split_commandline(result["launch_options"])[3]).is_file())
        self.stop.assert_not_called()

    def test_multiple_steam_accounts_return_manual_without_changes(self):
        other = self.steam.parent / "userdata/456/config/localconfig.vdf"
        other.parent.mkdir(parents=True)
        other.write_bytes(localconfig())
        before = self.config.read_bytes()
        result = self._configure()
        self.assertFalse(result["ok"])
        self.assertTrue(result["manual"])
        self.assertEqual(self.config.read_bytes(), before)
        self.stop.assert_not_called()

    def test_configure_game_b_preserves_game_a_wrapper_profile_and_backup(self):
        result_a = self._configure()
        self.assertTrue(result_a["ok"])
        before_vdf = self.config.read_bytes()
        snapshot_a = Path(result_a["profile_snapshot"])
        before_snapshot = snapshot_a.read_bytes()
        backup_path = Path(result_a["backup"])
        before_backup = backup_path.read_bytes()
        draft = self.root / "work/windows-profile.json"
        draft.write_bytes(b"GUI draft must stay unchanged")
        exe_b = self.game.parent / "Game B/game.exe"
        exe_b.parent.mkdir()
        exe_b.write_bytes(b"another game")
        (self.manifest.parent / "appmanifest_101.acf").write_text(
            '"AppState" { "appid" "101" "installdir" "Game B" }', encoding="utf-8")
        result_b = self._configure(replace(self.profile, game_exe=exe_b))
        self.assertFalse(result_b["ok"])
        self.assertIn("unresolved Steam backup", result_b["error"])
        self.assertEqual(self.config.read_bytes(), before_vdf)
        self.assertEqual(snapshot_a.read_bytes(), before_snapshot)
        self.assertEqual(backup_path.read_bytes(), before_backup)
        self.assertEqual(draft.read_bytes(), b"GUI draft must stay unchanged")
        self.assertEqual(self.stop.call_count, 1)

    def test_changed_profile_cannot_silently_reconfigure_existing_wrapper(self):
        self.assertTrue(self._configure()["ok"])
        before = self.config.read_bytes()
        changed = replace(self.profile, game_args=["+set", "another", "1"])
        result = self._configure(changed)
        self.assertFalse(result["ok"])
        self.assertEqual(self.config.read_bytes(), before)

    def test_restore_conflict_preserves_foreign_launch_options_and_backup(self):
        result = self._configure()
        backup_path = Path(result["backup"])
        before_backup = backup_path.read_bytes()
        updated = launch._edit_options(self.config.read_bytes(), "379720", "user's new options")
        self.config.write_bytes(updated)
        restored = launch.restore_steam(self.profile)
        self.assertFalse(restored["ok"])
        self.assertEqual(self.config.read_bytes(), updated)
        self.assertEqual(backup_path.read_bytes(), before_backup)
        self.assertEqual(self.stop.call_count, 1)

    def test_restore_recovers_committed_vdf_when_metadata_update_failed(self):
        before = self.config.read_bytes()
        result = self._configure()
        backup_path = Path(result["backup"])
        self.config.write_bytes(before)
        restored = launch.restore_steam(self.profile)
        self.assertTrue(restored["ok"], restored)
        self.assertTrue(restored["metadata_recovered"])
        self.assertFalse(restored["changed"])
        self.assertTrue(json.loads(backup_path.read_text(encoding="utf-8"))["restored"])
        self.assertEqual(self.config.read_bytes(), before)
        self.assertEqual(self.stop.call_count, 1)

    def test_malformed_owner_and_launch_state_fail_before_trigger_or_process(self):
        owner = self.game / "dlss-nr/.windows-wizard-owner.json"
        owner.write_text('[]', encoding="utf-8")
        self.assertFalse(launch.launch(self.profile)["ok"])
        self._install_marker(self.profile)
        state = self.root / "work/windows-wizard/launch-state.json"
        state.parent.mkdir()
        state.write_text('[]', encoding="utf-8")
        self.assertFalse(launch.launch(self.profile)["ok"])
        self.assertEqual(self.trigger.read_bytes(), b"on")
        self.core.set_effect.assert_not_called()
        self.popen.assert_not_called()

    def test_unknown_backup_format_is_preserved(self):
        backup = self.root / "work/windows-wizard/steam-backup.json"
        backup.parent.mkdir()
        backup.write_bytes(b'[]')
        before = self.config.read_bytes()
        result = self._configure()
        self.assertFalse(result["ok"])
        self.assertEqual(backup.read_bytes(), b'[]')
        self.assertEqual(self.config.read_bytes(), before)
        self.stop.assert_not_called()

    def test_concurrent_launch_options_change_during_shutdown_is_preserved(self):
        changed = launch._edit_options(self.config.read_bytes(), "379720", "+set foreign 1")
        def shutdown(*args):
            self.config.write_bytes(changed)
            return True
        self.stop.side_effect = shutdown
        result = self._configure()
        self.assertFalse(result["ok"])
        self.assertEqual(self.config.read_bytes(), changed)
        self.restart.assert_called_once_with(self.steam)

    def test_steam_restart_error_does_not_hide_configured_state(self):
        self.stop.return_value = True
        self.restart.side_effect = OSError("restart failed")
        result = self._configure()
        self.assertFalse(result["ok"])
        self.assertTrue(result["configured"])
        self.assertEqual(result["steam_restart_error"], "restart failed")
        self.assertEqual(launch._options(self.config.read_bytes(), "379720"), result["launch_options"])

    def test_restore_restart_error_does_not_hide_restored_state(self):
        before = self.config.read_bytes()
        self.assertTrue(self._configure()["ok"])
        self.stop.return_value = True
        self.restart.side_effect = OSError("restart failed")
        result = launch.restore_steam(self.profile)
        self.assertFalse(result["ok"])
        self.assertTrue(result["changed"])
        self.assertEqual(self.config.read_bytes(), before)

    def test_steam_games_guard_uses_other_libraries_and_running_app_id(self):
        folders = self.steam.parent / "steamapps/libraryfolders.vdf"
        folders.parent.mkdir()
        library = self.base / "another library"
        folders.write_text('"libraryfolders" { "1" { "path" ' + launch._vdf_quote(str(library)) + ' } }',
                           encoding="utf-8")
        self.processes.return_value = [{"Name": "other.exe", "ProcessId": 100,
                                      "ExecutablePath": str(library / "steamapps/common/Other/other.exe")}]
        self.assertEqual(len(launch._steam_games(self.steam, self.profile)), 1)
        self.processes.return_value = []
        with patch.object(launch, "_registry", return_value=123):
            self.assertEqual(len(launch._steam_games(self.steam, self.profile)), 1)

    def test_active_steam_game_never_shuts_down_client(self):
        with patch.object(launch, "_steam_games", return_value=[{"ProcessId": 100}]):
            with self.assertRaises(launch.LaunchError):
                ORIGINAL_STOP(self.steam, self.profile, None)
        self.popen.assert_not_called()

    def test_lingering_running_app_id_is_waited_for_and_then_refused(self):
        lingering = [{"Name": "Steam RunningAppID 379720", "ProcessId": None}]
        with patch.object(launch, "_steam_games", return_value=lingering), \
                patch.object(launch, "_registry", side_effect=[379720, 0]), \
                patch.object(launch, "_steam_running", return_value=False), \
                patch.object(launch.time, "sleep"):
            self.assertFalse(ORIGINAL_STOP(self.steam, self.profile, None))
        with patch.object(launch, "_steam_games", return_value=lingering), \
                patch.object(launch, "_registry", return_value=379720), \
                patch.object(launch.time, "sleep"), \
                patch.object(launch.time, "monotonic", side_effect=[0, 1, 16]):
            with self.assertRaises(launch.LaunchError):
                ORIGINAL_STOP(self.steam, self.profile, None)
        self.popen.assert_not_called()

    def test_steam_exit_is_graceful_and_never_force_kills(self):
        with patch.object(launch, "_steam_games", return_value=[]), \
                patch.object(launch, "_steam_running", side_effect=[True, True, False]), \
                patch.object(launch.time, "sleep"):
            self.assertTrue(ORIGINAL_STOP(self.steam, self.profile, None))
        self.assertEqual(self.popen.call_args.args[0], [str(self.steam), "-shutdown"])
        self.process.terminate.assert_not_called()
        self.process.kill.assert_not_called()

    def test_steam_shutdown_timeout_never_force_kills_or_edits(self):
        before = self.config.read_bytes()
        with patch.object(launch, "_steam_games", return_value=[]), \
                patch.object(launch, "_steam_running", return_value=True), \
                patch.object(launch.time, "monotonic", side_effect=[0, 21]):
            with self.assertRaises(launch.LaunchError):
                ORIGINAL_STOP(self.steam, self.profile, None)
        self.assertEqual(self.config.read_bytes(), before)
        self.process.terminate.assert_not_called()
        self.process.kill.assert_not_called()

    def test_normal_steam_restart_cleans_nr_environment(self):
        with patch.dict(os.environ, {"NR_ROOT": "old", "NR_PYTHON": "old-python", "NORMAL": "keep",
                                     "ENABLE_NR_LAYER": "1", "VK_LAYER_PATH": "old/dlss-nr",
                                     "DISABLE_VK_LAYER_VALVE_steam_fossilize_1": "1"}):
            self.assertEqual(ORIGINAL_RESTART(self.steam), 456)
        kwargs = self.popen.call_args.kwargs
        self.assertNotIn("NR_ROOT", kwargs["env"])
        self.assertNotIn("NR_PYTHON", kwargs["env"])
        self.assertNotIn("ENABLE_NR_LAYER", kwargs["env"])
        self.assertNotIn("VK_LAYER_PATH", kwargs["env"])
        self.assertEqual(kwargs["env"]["NORMAL"], "keep")

    def test_wrapper_cli_records_exit_and_rejects_mutable_gui_profile(self):
        snapshot = launch._ensure_steam_profile(self.profile)
        self.process.wait.return_value = 7
        code = launch.main(["--steam-wrapper", str(snapshot), "--", str(self.default_exe), "+set", "old", "1"])
        self.assertEqual(code, 7)
        state = json.loads((self.root / "work/windows-wizard/launch-state.json").read_text(encoding="utf-8"))
        self.assertEqual(state["exit_code"], 7)
        self.assertIn("ended_utc", state)
        draft = self.root / "work/windows-profile.json"
        draft.write_text(json.dumps(self.profile.to_dict()), encoding="utf-8")
        self.popen.reset_mock()
        with patch.object(launch.sys, "stderr"):
            self.assertEqual(launch.main(["--steam-wrapper", str(draft), "--", str(self.default_exe)]), 1)
        self.popen.assert_not_called()
        failure = self.root / "work/windows-wizard/steam-wrapper-last-error.json"
        self.assertTrue(failure.is_file())
        self.assertIn("immutable release snapshot", json.loads(failure.read_text(encoding="utf-8"))["error"])


ORIGINAL_STOP = launch._stop_steam
ORIGINAL_RESTART = launch._restart_steam


if __name__ == "__main__":
    unittest.main()
