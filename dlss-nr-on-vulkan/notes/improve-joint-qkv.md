# Joint Q/K/V preparation and measured diagnostics

`NR_JOINT_QKV=1` records normalization of Q, normalization of K and the V split
in one dispatch. The workgroups occupy three independent Y planes and reuse the
existing projection gathers, half-precision reduction tree and E4M3 publishing.
There is no new attention approximation, change to scores/softmax, or intermediate
buffer layout. The fifth pointer (query scales) uses byte offset 80 of the existing
push-constant block, which the C runtime checks with a static assertion.

The experiment requires the direct Q/K path (`NR_FUSE_QK=1`, already default).
The old split-first reference still works with `NR_FUSE_QK=0`; the benchmark rejects
that setting when asked to compare joint QKV, so it cannot silently benchmark the
same path twice. A separate graph-key bit preserves captured commands on toggles.

## Result: fewer dispatches did not mean faster on LNL

16 alternating pairs, input 440x248 padded to network 448x320. This is the network
extent corresponding to the reported B570 game window 800x450 at render_scale=0.55,
assuming no letterbox crop. Both runs here are on Intel Graphics (LNL / Arc 140V),
with FFN batching enabled and the compact input/output experiments disabled:

| joint QKV | recorded passes | median wall ms | min..max ms |
| --- | ---: | ---: | --- |
| off | 1128 | 79.050 | 77.003..82.641 |
| on | 988 | 83.213 | 81.702..85.043 |

Every output matched bit for bit. The 70 preparation sites remove two dispatches
each, but the median frame time increased by 4.163 ms, about 5.3%. The old Q/K/V
dispatches were already marked independent; combining kernels can also change
register allocation and scheduling. The benchmark does not isolate the cause.

**The option remains OFF by default.** This is a tested alternative for comparing
other hardware, not a claimed speedup. The timed interval includes host input write,
graph completion and output read; it excludes feature assembly, composition and game
rendering. B570/B580 performance is still unmeasured.

## Reproduce

```sh
make
python3 src/gpu/test_joint_qkv.py
XMX_STAGING=1 python3 src/gpu/test_joint_qkv.py
python3 src/bench/ffn_batch.py --optimization qkv --size 248 440 --pairs 16
```

For a per-operation diagnostic on the same network extent:

```sh
python3 src/bench/frame_profile.py --size 320 448 --runs 5
NR_JOINT_QKV=1 python3 src/bench/frame_profile.py --size 320 448 --runs 5
```

Profiling inserts GPU timestamps and records commands each run. Use the paired
replay benchmark above to judge speed; do not treat instrumented wall time as game FPS.
To try the option in a game, set it in the daemon's environment and restart the daemon.

## Diagnostics

- `nr-ctl status`, `set scale` and `nr-panel` no longer present the old 140V
  scale-only interpolation as an estimate for the user's GPU and window.
- `nr-ctl rates` explicitly labels the old measurements as an Arc 140V reference.
- Each actual frame log includes the network width/height and applied scale.
- The daemon logs its effective runtime switches, including joint QKV.
- `nr-ctl report` preserves complete frame lines and reports only the latest
  daemon session, rather than mixing old hardware/settings with current frames.
- Blur messages no longer claim a universal 7x slowdown. They state that the extra
  blur is used only when a detail/colour strength differs from 1. The OpenCV status
  describes the control tool's interpreter, which can differ from the daemon's.

## Validation

24 GPU/CPU comparisons passed, including partial workgroups and output guards.
Full frames at 320x320 and 448x320 matched exactly across specialization on/off,
replay/single/block execution, toggles, changed inputs and the split-first reference.
The same tests passed with XMX_STAGING=1. The CMake build and all 26 CTest entries
passed on LNL in 94.26 seconds. Diagnostic checks cover complete long log lines,
runtime options, last-session isolation, and absence of the old FPS/blur guesses.
