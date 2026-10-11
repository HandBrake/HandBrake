# Phase 41 — the layer works under VKD3D-Proton, and why DOA6LR does not start

2026-09-10.

## The result that matters for this project

**The Vulkan layer attaches to a D3D12 game under VKD3D-Proton.** Until now it had only
ever been driven through DXVK on a 32-bit D3D9 title (`notes/phase24`, `phase34`), and
VKD3D is a different Vulkan client entirely. Attached to DOA6LR it reported:

```
[nr_layer] active; socket=/tmp/nr_layer.sock trigger=/tmp/nr_trigger
[nr_layer] swapchain 800x450 format 44, 3 images
[nr_layer] the daemon did not answer; frame unchanged      (x4)
```

Format 44 is `VK_FORMAT_B8G8R8A8_UNORM`, one of the five the daemon decodes, so the
new four-bytes-a-pixel guard (`notes/phase39`) passes it. Four presents were
intercepted. Nothing about the 64-bit path or the D3D12 swapchain needed changing.

So the photo mode is ready for D3D12 titles. What is not ready is this particular game.

## Where DOA6LR dies

Verified from a Proton log, a Wine `+file` trace, and our own layer:

| stage | result |
| --- | --- |
| module load, `SteamAPI_Init` | OK |
| reading `fdata_package/*.fdata` | OK — the data layer works |
| `D3D12CreateDevice` via vkd3d-proton | OK — **DX Ultimate, SM 6.8, DXR 1.1** |
| swapchain 800x450, 3 images | OK — 4 frames presented |
| `ChangeProperties: Reallocating swapchain (1920 x 1200)` | logged |
| anything after that | **nothing, for 4.4 s** |
| exit | orderly: Streamline plugins unloaded, no exception |

The game asks to go fullscreen and then stops presenting. vkd3d recreates a swapchain
inside the present task, so no present means no recreation — our layer never saw a
1920x1200 swapchain either. Four seconds later the process shuts down cleanly. There is
no `c0000005`, no Vulkan error, no failed file open in the game's own tree.

## What it is not

Each of these was tested and changed nothing — same stage, same 4.4 s, same exit:

- **Proton version.** Hotfix in a fresh prefix fails identically to Experimental.
- **Saved graphics settings.** Moved `GRAPHICSSETTING` aside; identical, and the game
  did not even rewrite it.
- **esync / fsync.** `PROTON_NO_ESYNC=1 PROTON_NO_FSYNC=1`; identical.
- **XWayland vs native Wayland.** `PROTON_ENABLE_WAYLAND=1`; identical.
- **The Streamline plugins.** `sl.dlss`, `sl.dlss_g`, `sl.reflex` disabled; identical.
- **Aspect ratio.** A 1280x720 (16:9) Wine virtual desktop fails at the same point as
  the 1920x1200 (16:10) panel, so the panel's shape is not it.
- **The GPU stack.** The device exposes every modern feature the title could want.

## The one real configuration bug, and why fixing it is not enough

`version.dll` and `winmm.dll` sitting next to the exe are **Ultimate ASI Loader** — the
PDB path inside them says so. Wine's default override order is builtin-first, so it
loaded its *own* `version.dll` and the injection chain never ran:

```
...\Dead or Alive 6 Last Round\VERSION.dll  ::  builtin      <- Wine's, not the game's
```

`WINEDLLOVERRIDES="version=n,b;winmm=n,b"` fixes that, and `nt_file_dupe.asi` then
loads. It also immediately fails:

```
3542: Loaded   ...\nt_file_dupe.asi : native
3543: Unloaded ...\nt_file_dupe.asi : native
```

Same millisecond, adjacent lines — `DllMain` returned FALSE. It hooks NT file APIs,
which is not something that survives Wine's ntdll.

Everything the game needs *from the system* works. What does not work is the
third-party shim bolted onto this build, and that is outside anything this project
controls. The deterministic, orderly, always-at-the-same-point exit fits that and fits
nothing else that was tested.

## The discriminating test, and what it settled

If the resize itself were fatal, making it a no-op would save the game. A Wine virtual
desktop of exactly the starting size — `explorer /desktop=doa6,800x450` — was the way to
try it. It could not be done, and the reason is the finding:

| virtual desktop | swapchain the game then asks for |
| --- | --- |
| 1280x720 | 1280x720 |
| 800x450 | **1920x1200** |

The game does not simply take the desktop size. Nor does the target matter: a request
for a swapchain *larger than the desktop it is running in* dies at the same instant, in
the same way, as one that matches the panel exactly.

Across twelve runs the failure is invariant to every graphics-side variable available —
Proton build, prefix, window driver, sync primitives, Streamline plugins, saved settings,
aspect ratio, desktop size, and target resolution. **An invariant like that is the
evidence: the graphics stack is not the cause.**

The exit code says the same thing. `ExitProcess(1)` every time, after an orderly unload
of the Streamline plugins and with no exception anywhere in the `+seh` trace. The game
is not being killed — it is deciding to fail and saying so.

## Standing conclusion

For this project's purposes **Dead or Alive 5 remains the live target** — it runs, the
pass runs inside it, and the untested piece (the interface mask on a moving HUD) needs
that game, not this one. DOA6LR is worth returning to only if a build appears whose
startup path works under Proton; the Streamline prize described in `phase37` — the
`kBufferTypeHUDLessColor` / depth / motion-vector tags, verified present in this game's
`sl.common.dll` — is unreachable while the game will not start.

## The cause, found (2026-09-10 evening)

`phase41` above stopped at "the third-party shim bolted onto this build". That was wrong,
and the thing that settled it was Streamline's own log — it honours `SL_LOG_LEVEL`,
`SL_LOG_PATH` and `SL_LOG_NAME` as environment variables, which nothing in the earlier
pass had tried.

```
[warn]  sl.dlss not supported on current hardware
[warn]  Disabling DLSS-G since it is not supported on current hardware
[warn]  Ignoring plugin 'sl.dlss' since it is not supported on this platform
[warn]  Ignoring plugin 'sl.dlss_g' since it is not supported on this platform
[error] slValidateFeatureContext: 'kFeatureDLSS_G' context is missing.
[error] initializePlugins: D3D or VK API hook is activated without device being created
        ... repeated 34 times, from 3.5 s to 8 s, until shutdown
```

**The game requires DLSS-G — frame generation — and will not start without it.** Streamline
correctly refuses to load `sl.dlss` and `sl.dlss_g` on non-NVIDIA hardware; the game asks
for the DLSS-G feature context regardless; it is missing; and the interposer then spins on
"hook is activated without device being created" while the game waits for a call that will
never return.

Two measurements from the same evening support this and rule out what was suspected before:

- **The process is blocked, not computing.** Sampling `/proc/<pid>/stat` across the stall:
  heavy CPU and a climb to **91 threads** for six seconds, then **0-1 ticks per 500 ms**
  for three and a half seconds, then teardown. Every one of the 79 surviving threads sits
  in `futex_wait_multiple` or `futex_do_wait`, all in state S. That is a userspace deadlock,
  not an anti-tamper VM burning cycles — which is what the `.bind` section and the
  entropy-8.0 `.text` had suggested.
- **The crack is not involved.** With SmokeAPI's logging on, its hooks for
  `UserHasLicenseForApp`, `BIsSubscribedApp`, `BIsDlcInstalled` and `GetDLCCount` are
  installed and **never called once**. The game never asks about ownership. The last Steam
  calls before the stall are `SteamController008` and `SteamInput006`.

### Everything tried, and it is a long list

Proton Experimental and Hotfix, a fresh prefix, esync and fsync off, XWayland and native
Wayland, the saved graphics settings removed, aspect ratios and desktop sizes from 800x450
to 1920x1200, a Wine virtual desktop, `PROTON_DISABLE_HIDRAW`, `PROTON_LIMIT_RESOLUTIONS`,
`PROTON_NO_XIM`, `PROTON_DISABLE_NVAPI`, `PROTON_FORCE_NVAPI`,
`DXVK_NVAPI_ALLOW_OTHER_DRIVERS`, the Steam overlay disabled two ways,
`VKD3D_DISABLE_EXTENSIONS=VK_KHR_present_wait`, `VKD3D_SWAPCHAIN_LATENCY_FRAMES=1`,
`VKD3D_SWAPCHAIN_IMAGES=2`, `VKD3D_SWAPCHAIN_PRESENT_MODE`, and a `dxvk.conf` reporting an
NVIDIA vendor and device ID. **Every one is identical**: the same stall at the same point.

`sl.interposer.dll` is statically imported — disabling it gives a different, earlier
failure (exit 53 at seven seconds, no swapchain at all), which is how we know it is
load-bearing rather than optional.

The vendor spoof failing is itself informative: Streamline's hardware check does not go
through the DXGI adapter, it goes through NGX, which probes the real driver.

### What would actually fix it

Not a setting. The game needs something that **implements** the DLSS feature so Streamline
loads a working plugin instead of refusing — which is exactly what **OptiScaler** does by
replacing `nvngx`/`sl.dlss` with an FSR- or XeSS-backed implementation. That is a project
of its own and has nothing to do with this one; it is also the only route short of an
NVIDIA GPU.

**For this project the conclusion is unchanged and now properly grounded**: the layer is
proven under VKD3D-Proton, and DOA6LR is not the game to prove it on. The game directory
was restored — the `dxvk.conf`, the SmokeAPI log and its logging flag are all gone.
