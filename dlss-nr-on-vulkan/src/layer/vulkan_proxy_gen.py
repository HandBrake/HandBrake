#!/usr/bin/env python3
"""Writes the export side of the vulkan-1.dll proxy (nr_vulkan_proxy.c).

The proxy has to export every function the Vulkan loader exports: a native Vulkan game imports
them by name, and it would not start if one were missing. `vulkan-1.exports` lists them, taken
from the loader that came with Vulkan SDK 1.4.357 (System32 and SysWOW64 export the same 265).
Each gets a wrapper with the SDK's own prototype, so the compiler checks the signature, and on
32-bit Windows the stdcall argument size comes out right.

    python src/layer/vulkan_proxy_gen.py [--include C:/VulkanSDK/<version>/Include] [--check]

Writes nr_vulkan_proxy_exports.h and nr_vulkan_proxy.def beside this file; --check only says
whether they are what the SDK's headers give.
"""
from __future__ import annotations

import argparse
import os
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
PROTOTYPE = re.compile(r"VKAPI_ATTR\s+(.+?)\s+VKAPI_CALL\s+(vk\w+)\(\s*(.*?)\);", re.S)


def prototypes(include):
    found = {}
    for header in ("vulkan_core.h", "vulkan_win32.h"):
        text = (include / "vulkan" / header).read_text(encoding="utf-8")
        for returns, name, params in PROTOTYPE.findall(text):
            params = " ".join(params.split())
            found[name] = (" ".join(returns.split()), "" if params == "void" else params)
    return found


def argument(param):
    """The name a parameter is passed by: the last identifier, before any array bound."""
    return re.findall(r"[A-Za-z_]\w*", param.split("[")[0])[-1]


def render(names, found):
    missing = [name for name in names if name not in found]
    if missing:
        raise SystemExit("no prototype for: " + ", ".join(missing))
    lines = ["/* Written by vulkan_proxy_gen.py from vulkan-1.exports and the Vulkan SDK's headers;",
             " * not edited by hand. One wrapper for each function the loader exports. */"]
    for name in names:
        returns, params = found[name]
        args = ", ".join(argument(p) for p in params.split(",")) if params else ""
        params = params or "void"
        if returns == "void":
            lines.append(f"NR_PROXY_VOID({name}, ({params}), ({args}))")
        else:
            failure = "VK_ERROR_INITIALIZATION_FAILED" if returns == "VkResult" else "0"
            lines.append(f"NR_PROXY({returns}, {name}, ({params}), ({args}), {failure})")
    header = "\n".join(lines) + "\n"
    # No LIBRARY line: the DLL is built as nr_vulkan_proxy.dll and only named vulkan-1.dll
    # beside a game, and a LIBRARY name that is not the output's draws LNK4070.
    definition = "EXPORTS\n" + "".join(f"    {name}\n" for name in names)
    return header, definition


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sdk = os.environ.get("VULKAN_SDK")
    parser.add_argument("--include", type=pathlib.Path, default=pathlib.Path(sdk) / "Include" if sdk else None)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.include is None:
        parser.error("no Vulkan SDK: set VULKAN_SDK or pass --include")
    names = (HERE / "vulkan-1.exports").read_text(encoding="utf-8").split()
    header, definition = render(names, prototypes(args.include))
    outputs = {HERE / "nr_vulkan_proxy_exports.h": header, HERE / "nr_vulkan_proxy.def": definition}
    if args.check:
        stale = [path.name for path, text in outputs.items()
                 if not path.exists() or path.read_text(encoding="utf-8") != text]
        print("stale: " + ", ".join(stale) if stale else f"up to date ({len(names)} exports)")
        return 1 if stale else 0
    for path, text in outputs.items():
        path.write_text(text, encoding="utf-8", newline="\n")
    print(f"wrote {len(names)} wrappers")
    return 0


if __name__ == "__main__":
    sys.exit(main())
