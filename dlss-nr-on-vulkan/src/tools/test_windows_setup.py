#!/usr/bin/env python3
"""Run the shipped Windows installer with local fixtures, without a GPU or game.

The executable-selection part of its generated launcher is executed in cmd.exe;
only the final START is replaced with an observer, so no fixture EXE is launched.
Set NR_WINDOWS_SETUP_LOG_DIR to retain each command's output and exit code.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest

import test_release as fixtures


@unittest.skipUnless(os.name == "nt", "Windows installer requires cmd.exe")
class WindowsSetupTests(unittest.TestCase):
    write = fixtures.ReleaseTests.write

    def setUp(self):
        fixtures.ReleaseTests.setUp(self)
        fixtures.release.assemble(self.root, self.target, "windows")
        weights = self.target / "work/mlxw/dlssnr-logical.safetensors"
        weights.parent.mkdir()
        weights.write_bytes(b"local installer fixture, not model weights")
        self.game = self.base / "game (path test)"
        self.game.mkdir()
        self.destination = self.game / "dlss-nr"
        self.environment = {key: value for key, value in os.environ.items()
                            if not key.startswith("NR_")}
        self.environment["NR_PYTHON"] = sys.executable
        self.command_count = 0

    def run_command(self, command):
        self.command_count += 1
        got = subprocess.run(command, capture_output=True, text=True, timeout=30,
                             env=self.environment, cwd=self.base)
        if os.environ.get("NR_WINDOWS_SETUP_LOG_DIR"):
            folder = Path(os.environ["NR_WINDOWS_SETUP_LOG_DIR"])
            folder.mkdir(parents=True, exist_ok=True)
            prefix = folder / f"{self._testMethodName}-{self.command_count}"
            prefix.with_suffix(".stdout.log").write_text(got.stdout, encoding="utf-8")
            prefix.with_suffix(".stderr.log").write_text(got.stderr, encoding="utf-8")
            prefix.with_suffix(".result.json").write_text(json.dumps({
                "command": command, "exit_code": got.returncode,
            }, indent=2), encoding="utf-8")
        return got

    def setup(self):
        return self.run_command([
            os.environ.get("COMSPEC", "cmd.exe"), "/d", "/v:off", "/c", "call",
            str(self.target / "setup.bat"), "--game", str(self.game), "--skip-weights",
        ])

    def assert_failed(self, got, code=3):
        self.assertEqual(got.returncode, code, got.stdout + got.stderr)
        self.assertIn("ERROR:", got.stdout + got.stderr)
        self.assertNotIn("Done.", got.stdout)
        self.assertFalse((self.destination / "launch-nr.bat").exists())

    def test_parentheses_install_and_generated_launcher_use_the_release(self):
        game_exe = self.game / "game executable (fixture).exe"
        game_exe.write_bytes(b"fixture only; never executed")
        got = self.setup()
        self.assertEqual(got.returncode, 0, got.stdout + got.stderr)
        self.assertIn("Done.", got.stdout)
        self.assertNotIn("ERROR:", got.stdout + got.stderr)
        manifest = json.loads((self.destination / "VkLayer_dlss_nr.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["layer"]["library_path"], str(self.destination / "nr_layer.dll"))
        self.assertEqual(manifest["layer"]["library_arch"], "64")
        self.assertFalse((self.destination / "work/mlxw").exists())
        for name in ("libxmx.dll", "libnr_image.dll", "libnr_alloc.dll", "half_probe.spv"):
            self.assertEqual((self.destination / "work" / name).read_bytes(),
                             (self.target / "work" / name).read_bytes())

        launcher = (self.destination / "launch-nr.bat").read_text()
        self.assertEqual(launcher.splitlines()[-1].rstrip(), 'start "" "%GAME_EXE%" %*')
        self.assertIn('set "GAME_EXE=%%~fF"', launcher)
        observer = self.base / "observe launcher.py"
        report = self.base / "launcher.json"
        keys = ("VK_LAYER_PATH", "VK_INSTANCE_LAYERS", "ENABLE_NR_LAYER", "NR_LAYER_SPAWN",
                "NR_LAYER_LIVE", "NR_LAYER_SOCKET", "NR_ROOT", "NR_PYTHON", "NR_SETTINGS",
                "NR_LAYER_LOG", "NR_LAYER_TRIGGER", "GAME_EXE")
        observer.write_text("import json, os, pathlib\n"
                            f"keys = {keys!r}\n"
                            f"pathlib.Path({str(report)!r}).write_text(json.dumps({{"
                            "'environment': {key: os.environ.get(key) for key in keys}, "
                            "'cwd': os.getcwd()}))\n", encoding="utf-8")
        # Keep every environment/selection/working-directory line unchanged. The
        # zero-byte executable fixture is observed, never passed to CreateProcess.
        harness = self.base / "observe launcher.bat"
        lines = launcher.splitlines()
        lines[-1] = f'"{sys.executable}" "{observer}"'
        harness.write_text("\n".join(lines) + "\n", encoding="utf-8")
        got = self.run_command([os.environ.get("COMSPEC", "cmd.exe"), "/d", "/v:off", "/c",
                                "call", str(harness)])
        self.assertEqual(got.returncode, 0, got.stdout + got.stderr)
        observed = json.loads(report.read_text())
        self.assertEqual(observed["cwd"], str(self.game))
        expected = {
            "VK_LAYER_PATH": str(self.destination), "VK_INSTANCE_LAYERS": "VK_LAYER_dlssnr_intel",
            "ENABLE_NR_LAYER": "1", "NR_LAYER_SPAWN": "1", "NR_LAYER_LIVE": "1",
            "NR_LAYER_SOCKET": r"\\.\pipe\nr_dlssnr_intel", "NR_ROOT": str(self.target),
            "NR_PYTHON": sys.executable, "NR_SETTINGS": str(self.target / "work/nr_settings.json"),
            "NR_LAYER_LOG": str(self.target / "work/nr_daemon.log"),
            "NR_LAYER_TRIGGER": str(self.target / "work/nr_trigger"), "GAME_EXE": str(game_exe),
        }
        self.assertEqual(observed["environment"], expected)

    def test_missing_layer_reports_error_in_parentheses_path(self):
        (self.target / "nr_layer.dll").unlink()
        self.assert_failed(self.setup(), code=1)
        self.assertFalse(self.destination.exists())

    def test_missing_weights_reports_error_before_installation(self):
        (self.target / "work/mlxw/dlssnr-logical.safetensors").unlink()
        self.assert_failed(self.setup())
        self.assertFalse(self.destination.exists())

    def test_missing_allocator_is_not_installed(self):
        (self.target / "work/libnr_alloc.dll").unlink()
        self.assert_failed(self.setup())
        self.assertFalse(self.destination.exists())

    def test_layer_destination_directory_is_not_a_successful_install(self):
        blocked = self.destination / "nr_layer.dll"
        blocked.mkdir(parents=True)
        self.assert_failed(self.setup())
        self.assertEqual(list(blocked.iterdir()), [])
        self.assertFalse((self.destination / "VkLayer_dlss_nr.json").exists())

    def test_source_layer_directory_is_not_a_layer_file(self):
        path = self.target / "nr_layer.dll"
        path.unlink()
        path.mkdir()
        self.assert_failed(self.setup(), code=1)
        self.assertFalse(self.destination.exists())

    def test_runtime_directories_are_not_required_files(self):
        for name in ("libnr_alloc.dll", "half_probe.spv"):
            with self.subTest(runtime=name):
                path = self.target / "work" / name
                saved = path.read_bytes()
                path.unlink()
                path.mkdir()
                try:
                    self.assert_failed(self.setup())
                    self.assertFalse(self.destination.exists())
                finally:
                    path.rmdir()
                    path.write_bytes(saved)

    def test_each_required_shader_must_be_present(self):
        # Take the authoritative inventory from the runtime sources, rather than
        # duplicating setup.bat's list: newly required shaders must fail here too.
        for name in sorted(fixtures.release.required_shaders(self.root)):
            with self.subTest(shader=name):
                path = self.target / "work" / name
                saved = path.read_bytes()
                path.unlink()
                try:
                    self.assert_failed(self.setup())
                    self.assertFalse(self.destination.exists())
                finally:
                    path.write_bytes(saved)

    def test_source_directory_cannot_be_a_file(self):
        path = self.target / "src/layer"
        path.rename(path.with_name("saved-layer"))
        path.write_bytes(b"a file cannot provide the layer modules")
        self.assert_failed(self.setup())
        self.assertFalse(self.destination.exists())

    def test_invalid_manifest_cannot_print_done(self):
        (self.target / "VkLayer_dlss_nr.json").write_text("invalid JSON")
        self.assert_failed(self.setup())

    def test_manifest_write_failure_cannot_print_done(self):
        (self.destination / "VkLayer_dlss_nr.json").mkdir(parents=True)
        self.assert_failed(self.setup())

    def test_directory_creation_failure_cannot_print_done(self):
        self.destination.write_bytes(b"a file blocks the install directory")
        self.assert_failed(self.setup())

    def test_source_copy_failure_cannot_print_done(self):
        (self.destination / "src").mkdir(parents=True)
        (self.destination / "src/layer").write_bytes(b"a file blocks a required directory")
        self.assert_failed(self.setup())

    def test_library_copy_failure_cannot_print_done(self):
        (self.destination / "work/libnr_alloc.dll").mkdir(parents=True)
        self.assert_failed(self.setup())


if __name__ == "__main__":
    unittest.main(verbosity=2)
