# Execution order recovered, and our decode validated against real RTX hardware
2026-09-07

Source: `skchen17/dlssnr-amd-lab` — `docs/OPERATOR_RECONSTRUCTION.md`,
`scripts/build_operator_model_pack.py`, `scripts/decode_dlssnr_weight_resource.py`.
Read, cross-checked, and reimplemented; nothing was copied.

## 1. Our container decode is byte-identical to a real RTX capture

Their document states that stripping the record headers and padding every payload to
a 512-byte boundary yields a 147 719 680-byte "model arena" with SHA-256
`A5513B18…BD4EE3E5`, **byte-for-byte identical to an arena previously captured from a
running RTX 50**. Reproduced from our own parse:

```
our arena  : 147,719,680 bytes
their arena: 147,719,680 bytes            SIZE MATCH
our sha256 : A5513B1845C98A486985ED04F38E66A1854CCE33C2ABA3A505866028BD4EE3E5
their sha  : A5513B1845C98A486985ED04F38E66A1854CCE33C2ABA3A505866028BD4EE3E5
                                          IDENTICAL
```

This matters more than a second opinion. notes/CLAUDE.md's hard constraint 2 says there is
no NVIDIA GPU anywhere accessible and therefore no way to obtain reference data from
the original binary. That is still true — but our decode is now **transitively
validated against what the real runtime loads into GPU memory**, through a third
party's hardware capture. Phase 3's ground truth just got an anchor it was never
expected to have.

Their decoder independently confirms every structural detail we derived:
`ALIGNMENT = 512`, `DTYPE_FP16 = 1`, record trailer `(0, 0, 1, 0)`, "153 named FP16
records". All of that we had already established from the bytes alone.

## 2. The execution order

`build_operator_model_pack.py` carries the 156-slot captured graph collapsed into 14
stages. Checked against our block inventory, slot counts match **exactly**:

| stage | slots | n | our blocks | weight records |
|---|---|---|---|---|
| clear | 0 | 1 | — | 0 |
| encoder_1h_32 | 1–5 | 5 | block0–4 | 5 |
| encoder_2h_64 | 6–9 | 4 | block5–8 | 4 |
| encoder_4h_128 | 10–15 | 6 | block9–14 | 6 |
| encoder_8h_256 | 16–23 | 8 | block15–22 | 8 |
| bottleneck_16h_512 | 24–56 | 33 | block23–30 | 33 |
| vit_1d | 57–98 | 42 | block31–38 | 40 (+2 repack kernels) |
| decoder_16h_512 | 99–131 | 33 | block39–47 | 33 |
| decoder_8h_256 | 132–139 | 8 | block48–55 | 8 |
| decoder_4h_128 | 140–145 | 6 | block56–61 | 6 |
| decoder_2h_64 | 146–149 | 4 | block62–65 | 4 |
| decoder_1h_32 | 150–153 | 4 | block66–69 | 4 |
| output_head | 154 | 1 | block70 | 2 |
| final_copy | 155 | 1 | — | 0 |

156 slots, 153 weight records; the difference is `clear`, two weightless repacks in
the 1-D bottleneck, and the final copy. The bottleneck's 33 slots are exactly our
33 sub-layers (7 blocks x 4 + 1 block x 5), and the 16h/512 decoder's 33 are
`dec_input_upsample` plus 8 x 4. **The container's record order is the execution
order.** That was the last missing piece of the forward pass.

## 3. What each side has

They have live RTX capture — per-slot launch parameters, grids, bindings, and
intermediate buffers, which we cannot obtain. We have the static side done more
completely: a Linux-native PE and resource parser, Authenticode verification, the
zstd/PTX module extraction with all 231 kernels demangled to their template
dimensions, and the internal block layout derived from the weights themselves.
Their `extract_dlssnr_weights.py` only lifts the raw resource; it does not parse the
container into tensors.

Neither side has the *names* of the individual matrices inside a packed block. They
sidestepped it by capturing kernel parameters from hardware. We have the sizes and
offsets, which is what the operators actually need.
