# DOOM (2016): the game's Vulkan renderer through Proton

2026-10-05. A bounded live gameplay check on the owner's Intel LNL system.
The tested source checkout in ProjectsClaude was `3d8951c`; the running game mapped
that tree's layer and used its existing daemon. The synchronized improve sources
match that checkout.

## Which path actually ran

The game process was `DOOMx64vk.exe`. Its executable was Proton Experimental's
`wine64-preloader`, and its mappings included `winevulkan.so`, `winevulkan.dll`
and `vulkan-1.dll`. This tests the game's own Vulkan renderer through Wine;
it does not test a native Linux game executable. It should not be classified as
the D3D9/11-to-DXVK or D3D12-to-VKD3D paths tested in the earlier games.

An initial direct launch used Proton Hotfix and a separate test daemon/socket.
The gameplay process actually observed later was Steam's Proton Experimental
launch, using the ProjectsClaude layer and the standard NR endpoints. Toggling
the private test trigger produced no frames. Inspecting the actual process's
environment and library mappings identified the mismatch; the successful run
toggled its standard trigger instead. The reason the launch routes differed was
not established.

The user loaded an existing campaign scene manually. Automated input was stopped
after KDE's remote-control permission requests interfered with it.

## Result

| Item | Observation |
| --- | --- |
| Swapchain | 1280x720, B8G8R8A8_UNORM |
| Source settings | render_scale 0.4, min_extent 128 |
| Network field | 512x320 |
| Gameplay check | about 20 seconds; 390 processed frames |
| Refused frames | 0 in the captured daemon-log segment |
| Device lost | no report in the captured segment |
| Daemon latency | median about 40 ms, rounded to 10 ms in the log |
| GPU graph | median 30 ms, range 26–34 ms on frames with a GPU timing |
| First frame | 700 ms, including cold setup |
| Game HUD before NR | 59–60 FPS |
| Game HUD with NR | 23, 20 and 20 FPS in three snapshots |
| Game HUD after NR | 60 FPS |

The effect was visible, and the daemon reported nonzero image change: median
sampled mean absolute RGB change 0.02381. The successful run completed without a rejected
frame or device-lost report. The snapshots are observations of the game's own
HUD, not benchmark averages. Particle and weapon motion, camera changes and
shots were present, so the screenshots are not aligned same-frame quality
comparisons.

The actual standard trigger was absent before the run and removed afterwards.
The settings file was compared byte for byte and was unchanged. The unused
private daemon and input helper were stopped; the user's gameplay process was
left running with the effect off.

## Evidence and remaining coverage

Raw screenshots, the captured daemon-log segment, process mappings and a JSON
summary remain locally under ignored `work/doom-review-20261005/`. `RESULT.md`
there records the artifact context; `result.json` holds the extracted values.
Raw process paths and game settings are not published in this note.

This establishes a working short gameplay case for the direct Vulkan renderer.
Long combat sessions, other scenes, native Linux executables and discrete Arc
hardware remain separate checks. The test does not establish a general picture
quality benefit or performance on B570/B580.
