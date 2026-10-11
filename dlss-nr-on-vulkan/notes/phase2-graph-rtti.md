# Phase 2 — model graph recovered from RTTI, VERIFIED

The DLL is MSVC-built with RTTI intact. 35 mangled class names in the `HNetCpp`
namespace spell out the architecture directly, no CUDA toolkit required. This is
much stronger evidence than kernel-name inference and it arrived before we needed
`cuobjdump` at all.

The inference engine is called **HNet**.

## Scaffolding

```
Network            CCNetwork
Block              CCSingleLayerBlock
Layer              OBParamWrapper
CubinBackend       CubinBackendNGX      CCMultiCubinBackend
```

Dispatch wrappers outside the namespace: `NGXCubin`, `NGXCubinCUDA`, `NGXCubinD3D11`,
`NGXCubinD3D12`, **`NGXCubinVulkan`**, `NGXCubinGeneric`, plus matching
`NGXCubinFeature*` types. NVIDIA's own build already has a Vulkan dispatch path;
the CUDA dependency is in the kernels, not in the engine structure.

## Swin branch — windowed attention, 16 heads, "split" variant

```
CCSplitSwin16HBlock
  CCSplitSwin16HQKVAttnLayer      fused QKV + attention
  CCSplitSwin16HProjLayer         output projection
  CCSplitSwin16HProjPoolLayer     projection + pooling
  CCSplitSwin16HFfwdLayer         feed-forward
  CCSplitSwin16HFfwdProjLayer     feed-forward projection
  CCSplitSwin16HFinalHeadLayer    final head
```

## Swin — "Tinlayout" fused layers, head counts 1 / 2 / 4 / 8

```
CCTinlayoutFusedSwin1HLayer   CCTinlayoutFusedSwin2HLayer
CCTinlayoutFusedSwin4HLayer   CCTinlayoutFusedSwin8HLayer
CCTinlayoutFusedPreBlockSwin1HLayer
CCTinlayoutFusedPostBlockSwin1HLayer
```

The `NH` suffix is the head count. A pyramid over 1/2/4/8/16 heads is consistent with
a hierarchical Swin encoder with per-stage head counts.

## ViT branch — two flavours, 2D and 1D

```
CCVitBlock                CCVit1DBlock
CCVitQKVLayer             CCVit1DQKVLayer
CCVitAttentionLayer       CCVit1DAttentionLayer
CCVitProjectionLayer      CCVit1DProjectionLayer
CCVitFfnExpandLayer       CCVit1DFfnExpandLayer
CCVitFfnContractLayer     CCVit1DFfnContractLayer
```

## Decoder

```
CCDecInputUpsampleLayer
```

## What this means for the port

Every layer in the taxonomy is a GEMM-family op — QKV projection, attention,
FFN expand/contract, output projection, upsample. **There is no exotic operator in
this list.** All of it maps onto the one cooperative matrix shape we have
(8×16×16 bf16 → fp32, `notes/hw-coopmat.md`), plus softmax, normalisation and
elementwise work in ordinary compute shaders.

This confirms the reported "Swin and ViT blocks, QKV projections" and turns it from
press coverage into a concrete layer inventory.

## Still unknown — next steps

1. **Layer ordering, widths, depths.** RTTI gives the vocabulary, not the graph.
   Recover from the `CCNetwork` construction code in `.text`, or from the structure
   of the `.rsrc` weight blob (tensor shapes imply widths).
2. **Window size / shift for the Swin stages.**
3. How the diffusion conditioning (motion vectors, temporal state, artistic-direction
   values) enters — no obvious layer for it in the taxonomy yet.
