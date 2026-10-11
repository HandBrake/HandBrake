#!/usr/bin/env python3
"""build_check — the Makefile and CMakeLists.txt must build the same things.

Two build systems write the same artifacts into `work/`, and one of them drifts whenever a
change lands in the other: on 2026-09-27 CMake built 10 of the 19 shaders the runtime loads,
compiled the native passes without OpenMP or `-fno-trapping-math`, and registered 13 fewer
tests than `make test` runs — while the README offered it as the equal of `make`. This fails
the suite instead:

- every shader the runtime loads is built by both, from the same source with the same defines;
- every script `make test` runs is registered with CTest;
- the native passes get the flags their contract rests on in both, and the same CPU floor.

In this tree the native passes split their rows on `nr_image.c`'s own thread pool, not
OpenMP (Apple's clang has none, MSVC's is 2.0), so the check is for the pool's threads.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
FAILURES = []


def check(name, ok, detail=""):
    print(f"  [{'ok  ' if ok else 'FAIL'}] {name}{'  ' + detail if detail else ''}", flush=True)
    if not ok:
        FAILURES.append(name)


def loaded_shaders():
    """The SPIR-V the runtime opens: the device path's modules and libxmx itself."""
    names = set()
    for path in ("src/gpu/xmxres.py", "src/gpu/xmx.py", "src/gpu/libxmx.c"):
        names |= set(re.findall(r"([A-Za-z0-9_]+\.spv)",
                                (ROOT / path).read_text(encoding="utf-8")))
    return names


def make_shaders(text):
    """{spv: (source, defines)} from the Makefile's rules."""
    rules = {}
    text = text.replace("\\\n", " ")                  # join continued prerequisite lines
    for match in re.finditer(r"^work/([A-Za-z0-9_]+\.spv):\s*(\S+)[^\n]*\n\t\$\(GLSL\)([^\n]*)",
                             text, re.M):
        defines = tuple(sorted(re.findall(r"-D\S+", match.group(3))))
        rules[match.group(1)] = (match.group(2), defines)
    return rules


def cmake_shaders(text):
    rules = {}
    for match in re.finditer(r"^nr_shader\(([^\s)]+)\s+([^\s)]+)([^)\n]*)\)", text, re.M):
        rules[match.group(1)] = (match.group(2), tuple(sorted(re.findall(r"-D\S+", match.group(3)))))
    return rules


def make_tests(text):
    block = text[text.index("\ntest:"):]
    block = block[:block.index("\n\n")]
    return set(re.findall(r"(?:python3|\$\(PYTHON\)) (src/\S+\.py)", block))


def cmake_tests(text):
    scripts = set()
    for match in re.finditer(r"foreach\(name ([^)]*)\)(.*?)endforeach\(\)", text, re.S):
        names = match.group(1).split()
        template = re.search(r"\$\{PROJECT_SOURCE_DIR\}/(src/\S*\$\{name\}\S*?\.py)", match.group(2))
        if template:
            scripts |= {template.group(1).replace("${name}", name) for name in names}
    scripts |= set(re.findall(r'nr_c?test\([^\n]*"\$\{PROJECT_SOURCE_DIR\}/(src/[^"$]+\.py)"', text))
    # this tree's CMake registers most scripts from one list, each by its path
    for match in re.finditer(r"set\(NR_PY_TESTS(.*?)\)", text, re.S):
        scripts |= set(re.findall(r"(src/\S+\.py)", match.group(1)))
    return scripts


def main():
    make = (ROOT / "Makefile").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    loaded = loaded_shaders()
    in_make, in_cmake = make_shaders(make), cmake_shaders(cmake)
    check(f"the runtime's {len(loaded)} shaders are all built by make",
          loaded <= set(in_make), ", ".join(sorted(loaded - set(in_make))))
    check(f"and all built by CMake", loaded <= set(in_cmake),
          ", ".join(sorted(loaded - set(in_cmake))))
    differ = sorted(name for name in set(in_make) & set(in_cmake) if in_make[name] != in_cmake[name])
    check("each from the same source with the same defines", not differ,
          "; ".join(f"{name}: make {in_make[name]} cmake {in_cmake[name]}" for name in differ))
    ran, registered = make_tests(make), cmake_tests(cmake)
    check(f"CTest registers every script `make test` runs ({len(ran)})", ran <= registered,
          ", ".join(sorted(ran - registered)))
    rule = re.search(r"^work/libnr_image(?:\.so|\$\(SO\)):[^\n]*\n((?:\t[^\n]*\n)+)", make,
                     re.M).group(1)
    for flag in ("-ffp-contract=off", "-fno-fast-math", "-fno-trapping-math"):
        check(f"the native passes get {flag} from both", flag in rule and flag in cmake)
    check("and the row pool's threads from both",
          "-pthread" in rule and "target_link_libraries(nr_image PRIVATE Threads::Threads)" in cmake)
    # A release copies the library to other machines, so neither may build it for this one:
    # the Makefile's x86-64 floor is a named level, the rule compiles with it, and CMake's
    # default for x86-64 is the same level (its NR_IMAGE_ARCH cache variable, empty elsewhere).
    floor = re.search(r"^NR_IMAGE_ARCH \?= (-march=\S+)$", make, re.M)
    floor = floor.group(1) if floor else None
    check("and the same CPU floor from both", bool(floor) and "$(NR_IMAGE_ARCH)" in rule
          and f'set(nr_image_arch_default "{floor}")' in cmake and floor != "-march=native"
          and "-march=native" not in rule, floor or "none")
    if FAILURES:
        print(f"\n{len(FAILURES)} FAILED: " + ", ".join(FAILURES), flush=True)
        return 1
    print("\nthe two build systems build the same things", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
