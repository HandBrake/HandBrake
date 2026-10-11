# Phase 34 — a face, from a real game

2026-09-10. DLSS-NR runs on a frame of **Dead or Alive 5 Last Round**, on an Intel Xe2
iGPU, on NVIDIA's own weights, and the result goes back into the game's swapchain.

```
[nr_layer] active                                   (libnr_layer32.so, in game.exe)
1280x720 B8G8R8A8_UNORM in 4.01s  change 0.02891    cold
1280x720 B8G8R8A8_UNORM in 0.92s  change 0.02804    warm, no PNG dump
```

The frame is the character-select screen: two large, well-lit faces — skin, hair, eyes —
which is what this model was trained on. On the face crops the change is **0.0428 and
0.0404**, against **0.0289** over the whole frame, and the top decile of the difference
is **4.3x** the rest. It acts where it should. Hair separates into strands out of a
smooth mass, eyelashes and eyebrows resolve, the forehead and mouth creases on Bayman
become readable, and the skin picks up texture that is not in the input.

`work/doa5/in.png`, `out.png`, `face_a.png`, `face_b.png`.

## What had to be right

**The 32-bit layer.** `game.exe` is `PE32 ... Intel i386` importing `d3d9.dll`, and the
first launch loaded `libnr_layer.so` into `explorer.exe` and `xalia.exe` — Wine's 64-bit
helpers — and **not into the game**. The manifest in the directory the launch options
point at listed only the 64-bit library. `prepare_layer.py` writes both, with
`library_arch` in a 1.2.1 manifest, and after a restart:

```
pid 190236  vulkan=10  layer=[libnr_layer32.so]
```

Confirming the process is 32-bit is one line — its maps carry `i386-linux-gnu` paths.
Confirming the environment arrived is another: `/proc/<pid>/environ` holds
`VK_LAYER_PATH`, `VK_INSTANCE_LAYERS` and `ENABLE_NR_LAYER`. Both are worth checking
before assuming anything about why a layer did not fire.

**Steam's own launch options work.** The `--proton` launcher exists for when they are
not set, but a game started from the client with the variables in its launch options
reaches the layer through pressure-vessel intact, `/tmp` socket included.

## Where the 0.92 s goes

| | |
|---|---|
| the graph | ~0.49 s |
| daemon: feature assembly, composition, socket | ~0.43 s |
| `--dump` writing two PNGs | +1.5 s |

So a press of the trigger is about a second with dumping off, and the network is half
of it. The rest is single-threaded numpy either side of the GPU — which is what a photo
mode can afford and a per-frame pass could not.

## Standing

notes/CLAUDE.md's Phase 5 — "wire into a real game" — is done, on the game the owner picked,
with the content the model exists for. What it is not is real time: `notes/phase25` and
`notes/phase33` between them say why, and no amount of kernel work changes it.
