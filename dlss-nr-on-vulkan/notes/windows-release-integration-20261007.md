# Windows release/setup integration checked on 2026-10-07

The publication target is `windows`. Its checked GitHub head, `e616afc`, is an
ancestor of the release/setup candidate `ade823a`. An independent clean checkout
on `windows` fast-forwarded to that candidate without a conflict. This brings the
required release builder, deploy/setup fixes and named-pipe handle cleanup with
the five Windows release/setup/control commits. Old experimental branch tips and
the uncommitted GPU work in the other checkout are outside this integration.

No native C/header/shader, layer ABI, model weight format or extraction algorithm
changes occur between `e616afc` and `ade823a`. Runtime/control changes use the
existing layer name, root-scoped paths, native x64 Python and the ten shared
`nr_knobs` definitions. A normal fast-forward push is sufficient; no force push,
rebase of published history, or new PR is needed.

## Fresh Windows verification

Hardware: Intel Arc 140V 8GB, driver 32.0.101.9033, Windows 11 Home 25H2
26200.9457. Interpreter: CPython 3.12.12 AMD64, NumPy 2.5.3, safetensors 0.8.0.
The native runtime, x64/x86 layers, shaders and x64 setup host were rebuilt with
MSVC 19.44 and Vulkan SDK 1.4.357.0. Build and release assembly exited 0. The three
ignored GCC-pragma warnings in the host image source are unchanged.

- Six Windows release/setup/controls/wizard/bridge/Steam-launch suites: 129 tests,
  123 passed and 6 skipped, no failures/errors. Skips are five Linux-only cases
  and one unavailable Windows symlink privilege. Intentional failure fixtures
  produced the expected errors and nonzero child exit codes.
- Publication, claims, build-table and generated-knob-document checks: exit 0.
- WPF self-test on the fresh package: 64 named controls, ten live NR controls and
  140 English/Russian translation keys, exit 0.
- Native host image: 353 byte-equivalence checks with the freshly loaded DLL.
  Allocator: 15 checks with the freshly loaded DLL. Model primitives and all 649
  weight tensors: 41 checks; one optional Torch comparison was unavailable.
- Real Windows named pipe: eight frame byte roundtrips, up to 3,686,400 bytes.
  Cache, shader-table, portable daemon subsets and x64/x86 manifest checks passed.
- Full toggle regression: 25 checks, exit 0 on the host. Its first sandbox run
  timed out in CPython's localhost TCP socketpair fallback, before daemon code;
  the host rerun resolved that environmental restriction without a code change.
- A bounded daemon using the freshly rebuilt DLLs passed the startup half probe,
  installed the NumPy allocator and processed three real synthetic 640x360 BGRA
  frames over its own named pipe, at scale 0.5 and network 320x320. All three
  921,600-byte replies preserved alpha, changed colour values and retained the
  full colour range. Rejections and GPU-loss messages were zero. Owned test
  processes were cleaned up.

The GPU check is inference/transport validation, not a game FPS benchmark. No new
gameplay or DXVK test is claimed. The previous DOOM Vulkan campaign reports remain
separate. B570/B580 hardware was unavailable. Private DLLs, model weights,
generated binaries and raw logs remain ignored and are not part of source push.

Commands, separate stdout/stderr, exit codes, package audit and publication result
are saved locally under `D:\NRonWindows\windows-publish-check-20261007`.
