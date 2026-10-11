#!/usr/bin/env python3
"""The vulkan-1.dll proxy that setup puts beside a game (nr_vulkan_proxy.c), on Windows.

Each case loads it in a Python of its own, since its DllMain runs once a process, and the
Python stands in for the game:
- named in nr-env.txt, however spelt, the process gets the variables, the launch is recorded with its pid
  and the daemon log's size, the folder it was started in is listed once, and Vulkan calls reach the loader;
- another executable in the same folder gets nothing, and Vulkan still works;
- DISABLE_NR_PROXY=1 turns it all off;
- NR_REAL_VULKAN sends the calls to the loader it names (a game's own copy, set aside).

    python src/layer/test_vulkan_proxy.py [path to nr_vulkan_proxy.dll]

The DLL is work/nr_vulkan_proxy.dll, which build_win.bat builds. Exits 77, skipped, off
Windows, from a 32-bit Python, or when the DLL is not built.
"""
import json
import os
import pathlib
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
FAILURES = []
CHILD = r'''
import ctypes, json, os, pathlib, sys
from ctypes import wintypes
folder = pathlib.Path(sys.argv[1]); case = sys.argv[2]
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi")
kernel32.GetCurrentProcess.restype = wintypes.HANDLE
kernel32.GetModuleFileNameW.argtypes = (wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD)
kernel32.GetEnvironmentVariableW.argtypes = (wintypes.LPCWSTR, wintypes.LPWSTR, wintypes.DWORD)
psapi.EnumProcessModules.argtypes = (wintypes.HANDLE, ctypes.POINTER(wintypes.HMODULE), wintypes.DWORD,
                                     ctypes.POINTER(wintypes.DWORD))
buffer = ctypes.create_unicode_buffer(32768)
kernel32.GetModuleFileNameW(None, buffer, 32768)
me = buffer.value
spelt = {"other": me + ".not-this-one", "spelling": me.upper()}.get(case, me)
lines = ["NR_GAME_EXE=" + spelt,
         "NR_ROOT=" + str(folder / "release"),
         "NR_LAYER_LOG=" + str(folder / "daemon.log"),
         "NR_LAUNCH_STATE=" + str(folder / "launch-state.json"),
         "NR_TEST_VALUE=it works " + "été"]
if case == "real":
    lines.append("NR_REAL_VULKAN=" + str(folder / "own" / "vulkan-1.dll"))
(folder / "dlss-nr").mkdir(exist_ok=True)
(folder / "dlss-nr" / "nr-env.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
proxy = ctypes.WinDLL(str(folder / "vulkan-1.dll"))
value = ctypes.create_unicode_buffer(256)
got = kernel32.GetEnvironmentVariableW("NR_TEST_VALUE", value, 256)
version = ctypes.c_uint32(0)
result = proxy.vkEnumerateInstanceVersion(ctypes.byref(version))
# Every vulkan-1.dll in the process: the proxy, and the loader it passes calls to.
loaded = []
modules = (wintypes.HMODULE * 1024)(); needed = wintypes.DWORD()
psapi.EnumProcessModules(kernel32.GetCurrentProcess(), modules, ctypes.sizeof(modules), ctypes.byref(needed))
for module in modules[:needed.value // ctypes.sizeof(wintypes.HMODULE)]:
    name = ctypes.create_unicode_buffer(32768)
    kernel32.GetModuleFileNameW(module, name, 32768)
    if name.value.lower().endswith("vulkan-1.dll"):
        loaded.append(name.value)
# Where the proxy looks for the real loader: SysWOW64 for a 32-bit process, by redirection.
system = ctypes.create_unicode_buffer(32768)
kernel32.GetSystemDirectoryW(system, 32768)
print(json.dumps({"exe": me, "pid": os.getpid(), "value": value.value if got else None,
                  "from_system": any(os.path.samefile(path, os.path.join(system.value, "vulkan-1.dll"))
                                     for path in loaded),
                  "result": result, "version": version.value, "loaded": loaded}))
'''


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def run(proxy, case, environment=None, runs=1):
    """What the last of `runs` processes saw, its launch record, and for each folder listed in
    start-folders.txt whether it is the one they were started in (None without the file)."""
    folder = pathlib.Path(tempfile.mkdtemp(prefix="nr proxy (test) "))
    try:
        shutil.copyfile(proxy, folder / "vulkan-1.dll")
        (folder / "daemon.log").write_bytes(b"x" * 1234)
        if case == "real":
            (folder / "own").mkdir()
            shutil.copyfile(pathlib.Path(os.environ["SystemRoot"]) / "System32" / "vulkan-1.dll",
                            folder / "own" / "vulkan-1.dll")
        started = folder / "started here"
        started.mkdir()
        env = dict(os.environ, **(environment or {}))
        env.pop("NR_TEST_VALUE", None)
        for _ in range(runs):
            done = subprocess.run([sys.executable, "-c", CHILD, str(folder), case], env=env, cwd=started,
                                  capture_output=True, text=True, timeout=60)
            if done.returncode:
                return {"error": done.stderr.strip()[-400:]}, None, None
        state = folder / "launch-state.json"
        record = json.loads(state.read_text(encoding="utf-8")) if state.exists() else None
        listed = folder / "dlss-nr" / "start-folders.txt"
        # Compared as files while they exist: MSYS2's Python joins paths with /.
        folders = ([os.path.isdir(line) and os.path.samefile(line, started)
                    for line in listed.read_text(encoding="utf-8").splitlines()]
                   if listed.exists() else None)
        return json.loads(done.stdout.strip().splitlines()[-1]), record, folders
    finally:
        shutil.rmtree(folder, ignore_errors=True)


def main():
    if os.name != "nt" or struct.calcsize("P") != 8:
        print("skipped: 64-bit Windows Python only")
        return 77
    proxy = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "work" / "nr_vulkan_proxy.dll"
    if not proxy.is_file():
        print(f"skipped: no {proxy} (tools\\build_win.bat builds it)")
        return 77

    print("the game's own process, started twice:")
    seen, record, folders = run(proxy, "game", runs=2)
    check("it ran", "error" not in seen, seen.get("error", ""))
    check("the variable is set, UTF-8 read right", seen.get("value") == "it works été", repr(seen.get("value")))
    check("a Vulkan call reaches the loader", seen.get("result") == 0 and seen.get("version", 0) >= (1 << 22),
          f"result {seen.get('result')}, version {seen.get('version')}")
    # Compared as files, in the child while they exist: MSYS2's Python joins paths with /.
    check("System32's loader is the one behind it", seen.get("from_system") is True, str(seen.get("loaded")))
    check("the launch is recorded", record is not None and record.get("pid") == seen.get("pid")
          and record.get("daemon_start_bytes") == 1234 and record.get("exit_code") is None
          and record.get("game_exe") == seen.get("exe"), str(record))
    check("the folder it was started in is listed, once", folders == [True], str(folders))

    print("the game's path spelt otherwise (upper case):")
    seen, record, folders = run(proxy, "spelling")
    check("it is still the game", seen.get("value") == "it works été" and record is not None,
          repr(seen.get("value")))

    print("another executable in the folder:")
    seen, record, folders = run(proxy, "other")
    check("nothing is set", "error" not in seen and seen.get("value") is None, str(seen.get("value")))
    check("Vulkan still works", seen.get("result") == 0)
    check("no launch is recorded, no folder listed", record is None and folders is None)

    print("DISABLE_NR_PROXY=1:")
    seen, record, folders = run(proxy, "game", {"DISABLE_NR_PROXY": "1"})
    check("nothing is set, nothing recorded", seen.get("value") is None and record is None
          and folders is None)
    check("Vulkan still works", seen.get("result") == 0)

    print("the game's own loader, set aside:")
    seen, record, folders = run(proxy, "real")
    check("calls go to the loader NR_REAL_VULKAN names",
          seen.get("result") == 0 and any(path.lower().endswith("\\own\\vulkan-1.dll") for path in seen.get("loaded", [])),
          str(seen.get("loaded")))

    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nall passed", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
