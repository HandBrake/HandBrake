#!/usr/bin/env python3
"""Shared controls without a GPU, game, Git installation or real named pipe."""
import contextlib
import importlib.machinery
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/layer"))
import nr_paths  # noqa: E402

loader = importlib.machinery.SourceFileLoader("nr_ctl_controls_test", str(ROOT / "src/layer/nr-ctl"))
spec = importlib.util.spec_from_loader(loader.name, loader)
ctl = importlib.util.module_from_spec(spec)
loader.exec_module(ctl)


class ControlsTests(unittest.TestCase):
    def setUp(self):
        self.room = tempfile.TemporaryDirectory(prefix="nr-controls-")
        self.addCleanup(self.room.cleanup)
        self.base = Path(self.room.name)
        self.selected = self.base / "selected release (root)"
        self.daemon = self.base / "tools/src/layer/nr_daemon.py"
        self.daemon.parent.mkdir(parents=True)
        self.daemon.write_text("# never executed by this test\n", encoding="utf-8")
        self.settings = self.selected / "work/settings.json"
        self.log = self.selected / "new log folder/daemon.log"
        self.pipe = Path(r"\\.\pipe\nr_controls_test")
        for name, value in (("DAEMON", self.daemon), ("SETTINGS", self.settings),
                            ("LOG", self.log), ("SOCKET", self.pipe)):
            patcher = mock.patch.object(nr_paths, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)
        patcher = mock.patch.dict(os.environ, {"NR_ROOT": str(self.selected)}, clear=False)
        patcher.start()
        self.addCleanup(patcher.stop)

    def pipe_error(self, code):
        error = OSError("named pipe probe")
        error.winerror = code
        return error

    def test_busy_pipe_is_alive_without_waiting(self):
        with mock.patch.object(nr_paths.os, "name", "nt"), \
             mock.patch("builtins.open", side_effect=self.pipe_error(231)) as opened:
            self.assertTrue(nr_paths.alive())
        opened.assert_called_once_with(str(self.pipe), "r+b", buffering=0)

    def test_missing_denied_and_other_pipe_errors_are_not_alive(self):
        for code in (2, 5, 109, 233):
            with self.subTest(winerror=code), mock.patch.object(nr_paths.os, "name", "nt"), \
                 mock.patch("builtins.open", side_effect=self.pipe_error(code)) as opened:
                self.assertFalse(nr_paths.alive())
                opened.assert_called_once()

    def test_successful_pipe_probe_closes_its_handle(self):
        with mock.patch.object(nr_paths.os, "name", "nt"), \
             mock.patch("builtins.open", mock.mock_open()) as opened:
            self.assertTrue(nr_paths.alive())
        opened.return_value.__exit__.assert_called_once()

    def test_linux_socket_probe_retains_bounded_connect(self):
        endpoint = self.base / "linux.sock"
        endpoint.touch()
        with mock.patch.object(nr_paths.os, "name", "posix"), \
             mock.patch.object(nr_paths, "SOCKET", endpoint), \
             mock.patch.object(nr_paths.socket, "AF_UNIX", 1, create=True), \
             mock.patch.object(nr_paths.socket, "socket") as factory:
            self.assertTrue(nr_paths.alive())
        probe = factory.return_value.__enter__.return_value
        probe.settimeout.assert_called_once_with(0.5)
        probe.connect.assert_called_once_with(str(endpoint))

    def test_start_uses_selected_root_python_and_isolates_vulkan_environment(self):
        chosen_python = str(self.base / "chosen Python (native)/python.exe")
        parent_env = {"NR_PYTHON": chosen_python, "VK_LAYER_PATH": "game layers",
                      "VK_ADD_LAYER_PATH": "more layers", "VK_IMPLICIT_LAYER_PATH": "implicit",
                      "VK_ADD_IMPLICIT_LAYER_PATH": "added implicit", "VK_INSTANCE_LAYERS": "VK_LAYER_dlssnr_intel",
                      "VK_LAYER_SETTINGS_PATH": "game settings", "ENABLE_NR_LAYER": "1",
                      "NR_DAEMON": "old daemon.py", "NR_LAYER_SPAWN": "1",
                      "NR_LAYER_LOG": str(self.log), "NR_LAYER_SOCKET": str(self.pipe),
                      "NR_SETTINGS": str(self.settings), "KEEP_OTHER": "value"}
        with mock.patch.dict(os.environ, parent_env), \
             mock.patch.object(nr_paths.subprocess, "Popen") as spawn, \
             mock.patch.object(nr_paths, "alive", return_value=True):
            self.assertIsNone(nr_paths.start_daemon(wait=0.5))
            self.assertEqual(os.environ["ENABLE_NR_LAYER"], "1")
        command = spawn.call_args.args[0]
        child = spawn.call_args.kwargs
        self.assertEqual(command[0], chosen_python)
        self.assertEqual(command[command.index("--root") + 1], str(self.selected))
        self.assertEqual(command[command.index("--socket") + 1], str(self.pipe))
        self.assertEqual(command[command.index("--settings") + 1], str(self.settings))
        self.assertTrue(self.log.is_file())
        self.assertEqual(json.loads(self.settings.read_text())["render_scale"], nr_paths.FIRST_SCALE)
        self.assertEqual(child["env"]["NR_ROOT"], str(self.selected))
        self.assertEqual(child["env"]["NR_LAYER_LOG"], str(self.log))
        self.assertEqual(child["env"]["NR_LAYER_SOCKET"], str(self.pipe))
        self.assertEqual(child["env"]["NR_SETTINGS"], str(self.settings))
        self.assertEqual(child["env"]["DISABLE_NR_LAYER"], "1")
        self.assertEqual(child["env"]["NR_LAYER_SPAWN"], "0")
        self.assertEqual(child["env"]["KEEP_OTHER"], "value")
        for key in ("VK_LAYER_PATH", "VK_ADD_LAYER_PATH", "VK_IMPLICIT_LAYER_PATH",
                    "VK_ADD_IMPLICIT_LAYER_PATH", "VK_INSTANCE_LAYERS", "VK_LAYER_SETTINGS_PATH",
                    "ENABLE_NR_LAYER", "NR_DAEMON"):
            self.assertNotIn(key, child["env"])
        self.assertEqual(child["stdin"], subprocess.DEVNULL)

    def test_frozen_control_does_not_relaunch_the_gui_as_a_daemon(self):
        with mock.patch.dict(os.environ, {"NR_PYTHON": ""}), \
             mock.patch.object(nr_paths.sys, "frozen", True, create=True), \
             mock.patch.object(nr_paths.subprocess, "Popen") as spawn:
            self.assertIn("NR_PYTHON", nr_paths.start_daemon())
        spawn.assert_not_called()

    def test_daemon_process_failure_is_reported_without_waiting_for_timeout(self):
        with mock.patch.object(nr_paths.subprocess, "Popen") as spawn, \
             mock.patch.object(nr_paths, "alive", return_value=False), \
             mock.patch.object(nr_paths.time, "sleep") as sleep:
            spawn.return_value.poll.return_value = 7
            self.assertIn("code 7", nr_paths.start_daemon(wait=10))
        sleep.assert_not_called()

    def test_daemon_wait_rejects_nonfinite_values_without_spawning(self):
        for wait in (float("inf"), float("nan"), -1, "invalid"):
            with self.subTest(wait=wait), mock.patch.object(nr_paths.subprocess, "Popen") as spawn:
                self.assertIn("finite", nr_paths.start_daemon(wait=wait))
                spawn.assert_not_called()

    def test_windows_wait_is_capped_and_cannot_sleep_past_deadline(self):
        with mock.patch.object(nr_paths.os, "name", "nt"), \
             mock.patch.object(nr_paths.subprocess, "CREATE_NO_WINDOW", 0x08000000, create=True), \
             mock.patch.object(nr_paths.subprocess, "Popen") as spawn, \
             mock.patch.object(nr_paths, "alive", return_value=False), \
             mock.patch.object(nr_paths.time, "monotonic", side_effect=[100, 100, 200, 200]), \
             mock.patch.object(nr_paths.time, "sleep") as sleep:
            spawn.return_value.poll.return_value = None
            self.assertIn("still loading", nr_paths.start_daemon(wait=1e9))
        sleep.assert_not_called()

    def report_output(self):
        output = io.StringIO()
        with mock.patch.object(ctl, "status") as status, contextlib.redirect_stdout(output):
            ctl.report()
        status.assert_called_once()
        return output.getvalue()

    def test_report_without_git_does_not_raise(self):
        with mock.patch.object(subprocess, "run", side_effect=FileNotFoundError("git absent")) as run:
            self.assertIn("Git unavailable", self.report_output())
        self.assertEqual(run.call_args.kwargs["timeout"], 2.0)

    def test_report_git_timeout_and_nonclone_are_graceful(self):
        outcomes = (subprocess.TimeoutExpired("git", 2), subprocess.CompletedProcess("git", 128, "", "not a clone"))
        for outcome in outcomes:
            with self.subTest(outcome=outcome):
                kwargs = {"side_effect": outcome} if isinstance(outcome, Exception) else {"return_value": outcome}
                with mock.patch.object(subprocess, "run", **kwargs):
                    self.assertIn("not a git clone", self.report_output())

    def test_report_uses_package_metadata_from_selected_root_without_git(self):
        self.selected.mkdir()
        metadata = {"schema_version": 1, "source_commit": "abcdef0123456789", "source_branch": "release branch"}
        (self.selected / "release-metadata.json").write_text(json.dumps(metadata), encoding="utf-8-sig")
        with mock.patch.object(subprocess, "run") as run:
            output = self.report_output()
        self.assertIn("release abcdef012345 (release branch)", output)
        run.assert_not_called()

    def test_report_incomplete_metadata_is_still_a_package_without_git(self):
        self.selected.mkdir()
        (self.selected / "release-metadata.json").write_text('{"schema_version": 1}', encoding="utf-8")
        with mock.patch.object(subprocess, "run") as run:
            self.assertIn("commit not recorded", self.report_output())
        run.assert_not_called()

    def test_report_unreadable_log_is_graceful(self):
        self.log.parent.mkdir(parents=True)
        self.log.touch()
        with mock.patch.object(subprocess, "run", side_effect=FileNotFoundError("git absent")), \
             mock.patch.object(type(self.log), "read_text", side_effect=PermissionError("log denied")):
            self.assertIn("could not read it", self.report_output())


if __name__ == "__main__":
    unittest.main(verbosity=2)
