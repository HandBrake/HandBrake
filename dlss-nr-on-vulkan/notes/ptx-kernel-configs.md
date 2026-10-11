# Kernel -> layer class -> template config (from PTX, 2026-09-07)

Generated from the demangled `tin3_1` shared-memory symbols in the extracted PTX.
231 kernels; 174 carry a layer template. Base name = kernel with the
`_fp8/_wait/_chained/_tilesync/_shifted/_inpview/_outview/_ds/_publish` suffixes stripped.

| kernel (base) | layer class | config parameters |
|---|---|---|
| `cc_dec_input_upsample_1024_512` | `Conv2d1x1Layer` | `<512, 1024, 4, 4, 256, 32, 1, 2, 4, false, false, false, 256>` |
| `cc_dec_input_upsample_1024_512` | `Conv2d1x1Layer` | `<512, 1024, 4, 4, 256, 64, 1, 2, 4, false, false, false, 256>` |
| `cc_dec_input_upsample_1024_512` | `Conv2d1x1Layer` | `<512, 1024, 4, 4, 256, 32, 1, 2, 4, false, false, true, 256>` |
| `cc_dec_input_upsample_1024_512` | `Conv2d1x1Layer` | `<512, 1024, 4, 4, 256, 64, 1, 2, 4, false, false, true, 256>` |
| `cc_split_swin_16h_ffwd_512` | `None` | `<512, 64, 4, 8, 8, 1, 4, 32, 32, false, false, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_512` | `None` | `<512, 64, 4, 8, 8, 1, 4, 32, 32, true, true, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_512` | `None` | `<512, 64, 4, 8, 8, 2, 4, 64, 32, true, true, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_512` | `None` | `<512, 64, 4, 8, 8, 2, 4, 64, 32, false, false, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_512` | `None` | `<512, 64, 4, 8, 8, 1, 4, 32, 32, false, true, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_512` | `None` | `<512, 64, 4, 8, 8, 2, 4, 64, 32, false, true, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_512` | `None` | `<512, 64, 4, 8, 8, 1, 4, 32, 32, true, false, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_512` | `None` | `<512, 64, 4, 8, 8, 2, 4, 64, 32, true, false, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_inpview_512` | `None` | `<512, 64, 4, 8, 8, 1, 4, 32, 32, false, false, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_inpview_512` | `None` | `<512, 64, 4, 8, 8, 1, 4, 64, 32, false, false, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_inpview_512` | `None` | `<512, 64, 4, 8, 8, 1, 4, 32, 32, false, true, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_inpview_512` | `None` | `<512, 64, 4, 8, 8, 1, 4, 64, 32, false, true, 256, 8, 8>` |
| `cc_split_swin_16h_ffwd_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, false, false, 256>` |
| `cc_split_swin_16h_ffwd_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, true, true, 256>` |
| `cc_split_swin_16h_ffwd_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, true, true, 256>` |
| `cc_split_swin_16h_ffwd_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, false, false, 256>` |
| `cc_split_swin_16h_ffwd_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, true, false, 256>` |
| `cc_split_swin_16h_ffwd_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, true, false, 256>` |
| `cc_split_swin_16h_ffwd_proj_inpview_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, false, false, 256>` |
| `cc_split_swin_16h_ffwd_proj_inpview_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, true, true, 256>` |
| `cc_split_swin_16h_ffwd_proj_inpview_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, true, true, 256>` |
| `cc_split_swin_16h_ffwd_proj_inpview_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, false, false, 256>` |
| `cc_split_swin_16h_final_head_512` | `Conv2d1x1Layer` | `<1024, 512, 8, 8, 256, 32, 2, 4, 1, false, false, false, 256>` |
| `cc_split_swin_16h_final_head_512` | `Conv2d1x1Layer` | `<1024, 512, 8, 8, 256, 64, 2, 4, 1, false, false, false, 256>` |
| `cc_split_swin_16h_final_head_512` | `Conv2d1x1Layer` | `<1024, 512, 8, 8, 256, 32, 2, 4, 1, false, true, false, 256>` |
| `cc_split_swin_16h_final_head_512` | `Conv2d1x1Layer` | `<1024, 512, 8, 8, 256, 64, 2, 4, 1, false, true, false, 256>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 2, 4, 1, true, false, false, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 2, 4, 1, true, true, true, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 2, 4, 1, true, true, true, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 2, 4, 1, true, false, false, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, false, false, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, false, false, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, true, false, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, true, false, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 2, 4, 1, true, false, true, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 2, 4, 1, true, false, true, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 2, 4, 1, true, true, false, 128>` |
| `cc_split_swin_16h_proj_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 2, 4, 1, true, true, false, 128>` |
| `cc_split_swin_16h_proj_pool_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, false, false, 128>` |
| `cc_split_swin_16h_proj_pool_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 32, 1, 4, 1, true, true, true, 128>` |
| `cc_split_swin_16h_proj_pool_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, true, true, 128>` |
| `cc_split_swin_16h_proj_pool_512` | `Conv2d1x1Layer` | `<512, 512, 8, 8, 256, 64, 1, 4, 1, true, false, false, 128>` |
| `cc_split_swin_16h_qkv_512` | `None` | `<512, 8, 8, 16, 4, 16, 4, false, false, 256, 8, 8>` |
| `cc_split_swin_16h_qkv_512` | `None` | `<512, 8, 8, 16, 4, 16, 4, true, true, 256, 8, 8>` |
| `cc_split_swin_16h_qkv_512` | `None` | `<512, 8, 8, 16, 4, 64, 4, true, true, 256, 8, 8>` |
| `cc_split_swin_16h_qkv_512` | `None` | `<512, 8, 8, 16, 4, 64, 4, false, false, 256, 8, 8>` |
| `cc_tinlayout_fused_pre_block_swin_1h_32_1` | `FusedSwin2d1HLayer` | `<32, 4, 8, 8, 1, 1, 8, 8, 4, 32, 32, 16, 16, 16, true, false, false, false, 64, 32, 32, 8, 8, 32, true, false, true, false, false, true, 16>` |
| `cc_tinlayout_fused_pre_block_swin_1h_32_1` | `FusedSwin2d1HLayer` | `<32, 4, 8, 8, 1, 1, 8, 8, 4, 32, 32, 16, 16, 16, true, false, false, false, 64, 32, 32, 8, 8, 32, true, false, true, true, false, true, 16>` |
| `cc_tinlayout_fused_pre_block_swin_1h_32_1` | `FusedSwin2d1HLayer` | `<32, 4, 8, 8, 1, 1, 8, 8, 4, 32, 32, 32, 32, 32, true, false, false, false, 64, 32, 32, 8, 8, 32, true, false, true, true, false, true, 16>` |
| `cc_tinlayout_fused_pre_block_swin_1h_32_1` | `FusedSwin2d1HLayer` | `<32, 4, 8, 8, 1, 1, 8, 8, 4, 32, 32, 32, 32, 32, true, false, false, false, 64, 32, 32, 8, 8, 32, true, false, true, false, false, true, 16>` |
| `cc_tinlayout_fused_swin_2h_64_2` | `CrazyCuckooFusedSwin2d2HLayer` | `<64, 32, 4, 8, 8, 2, 2, 8, 8, 32, 32, 32, 2, false, false, false, 64, 8, 8, 64, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_2h_64_2` | `CrazyCuckooFusedSwin2d2HLayer` | `<64, 32, 4, 8, 8, 2, 2, 8, 8, 32, 32, 32, 2, false, true, true, 64, 8, 8, 64, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_2h_64_2` | `CrazyCuckooFusedSwin2d2HLayer` | `<64, 32, 4, 8, 8, 2, 2, 8, 8, 32, 32, 32, 2, false, false, false, 64, 8, 8, 64, true, false, true, true, false>` |
| `cc_tinlayout_fused_swin_2h_64_2` | `CrazyCuckooFusedSwin2d2HLayer` | `<64, 32, 4, 8, 8, 2, 2, 8, 8, 32, 32, 32, 2, false, true, false, 64, 8, 8, 64, true, false, true, true, false>` |
| `cc_tinlayout_fused_swin_2h_64_2` | `CrazyCuckooFusedSwin2d2HLayer` | `<64, 32, 4, 8, 8, 2, 2, 8, 8, 32, 32, 32, 2, false, false, false, 64, 8, 8, 64, true, false, true, false, true>` |
| `cc_tinlayout_fused_swin_2h_64_2` | `CrazyCuckooFusedSwin2d2HLayer` | `<64, 32, 4, 8, 8, 2, 2, 8, 8, 32, 32, 32, 2, false, false, true, 64, 8, 8, 64, true, false, true, false, true>` |
| `cc_tinlayout_fused_swin_2h_64_2` | `CrazyCuckooFusedSwin2d2HLayer` | `<64, 32, 4, 8, 8, 2, 2, 8, 8, 32, 32, 32, 2, false, true, false, 64, 8, 8, 64, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_2h_64_2` | `CrazyCuckooFusedSwin2d2HLayer` | `<64, 32, 4, 8, 8, 2, 2, 8, 8, 32, 32, 32, 2, false, false, true, 64, 8, 8, 64, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_2h_64_2_upsample` | `CrazyCuckooFusedSwin2d4HLayer` | `<64, 32, 4, 8, 8, 2, 8, 8, 32, 32, 32, 2, true, false, false, 128, 4, 4, 64, true, true, true, false, false>` |
| `cc_tinlayout_fused_swin_2h_64_2_upsample` | `CrazyCuckooFusedSwin2d4HLayer` | `<64, 32, 4, 8, 8, 2, 8, 8, 32, 32, 32, 2, true, false, true, 128, 4, 4, 64, true, true, true, false, false>` |
| `cc_tinlayout_fused_swin_4h_128_4` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, false, false, false, 128, 8, 8, 128, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_4h_128_4` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, false, true, true, 128, 8, 8, 128, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_4h_128_4` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, false, false, false, 128, 8, 8, 128, true, false, true, true, false>` |
| `cc_tinlayout_fused_swin_4h_128_4` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, false, true, false, 128, 8, 8, 128, true, false, true, true, false>` |
| `cc_tinlayout_fused_swin_4h_128_4` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, false, false, false, 128, 8, 8, 128, true, false, true, false, true>` |
| `cc_tinlayout_fused_swin_4h_128_4` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, false, false, true, 128, 8, 8, 128, true, false, true, false, true>` |
| `cc_tinlayout_fused_swin_4h_128_4` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, false, true, false, 128, 8, 8, 128, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_4h_128_4` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, false, false, true, 128, 8, 8, 128, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_4h_128_4_upsample` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, true, false, false, 256, 4, 4, 128, true, true, true, false, false>` |
| `cc_tinlayout_fused_swin_4h_128_4_upsample` | `CrazyCuckooFusedSwin2d4HLayer` | `<128, 32, 4, 8, 8, 4, 8, 8, 32, 32, 32, 4, true, false, true, 256, 4, 4, 128, true, true, true, false, false>` |
| `cc_tinlayout_fused_swin_8h_256_8` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, false, false, false, 256, 8, 8, 256, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_8h_256_8` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, false, true, true, 256, 8, 8, 256, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_8h_256_8` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, false, false, false, 256, 8, 8, 256, true, false, true, true, false>` |
| `cc_tinlayout_fused_swin_8h_256_8` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, false, true, false, 256, 8, 8, 256, true, false, true, true, false>` |
| `cc_tinlayout_fused_swin_8h_256_8` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, false, false, false, 256, 8, 8, 256, true, false, true, false, true>` |
| `cc_tinlayout_fused_swin_8h_256_8` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, false, false, true, 256, 8, 8, 256, true, false, true, false, true>` |
| `cc_tinlayout_fused_swin_8h_256_8` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, false, true, false, 256, 8, 8, 256, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_8h_256_8` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, false, false, true, 256, 8, 8, 256, true, false, true, false, false>` |
| `cc_tinlayout_fused_swin_8h_256_8_upsample` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, true, false, false, 512, 4, 4, 256, true, true, true, false, false>` |
| `cc_tinlayout_fused_swin_8h_256_8_upsample` | `CrazyCuckooFusedSwin2d4HLayer` | `<256, 32, 4, 8, 8, 8, 8, 8, 32, 32, 32, 8, true, false, true, 512, 4, 4, 256, true, true, true, false, false>` |
| `cc_vit_1d_attention` | `Attention1dLayer` | `<32, 32, 128, 64, 32, 2, false, false>` |
| `cc_vit_1d_attention` | `Attention1dLayer` | `<32, 32, 128, 64, 32, 2, true, false>` |
| `cc_vit_1d_projection` | `Conv1d1x1Layer` | `<1024, 1024, 128, 128, 32, 2, 2, 4, true, false, false, 32>` |
| `cc_vit_1d_projection` | `Conv1d1x1Layer` | `<1024, 1024, 128, 128, 32, 2, 2, 4, true, true, true, 32>` |
| `cc_vit_1d_projection` | `Conv1d1x1Layer` | `<1024, 1024, 128, 128, 32, 2, 2, 4, true, true, false, 32>` |
| `cc_vit_1d_qkv` | `Conv1dQKVLayer` | `<32, 32, 1024, 128, 32, 2, 2, 2, false, false, 128>` |
| `cc_vit_1d_qkv` | `Conv1dQKVLayer` | `<32, 32, 1024, 128, 32, 2, 2, 2, true, false, 128>` |
| `cc_vit_attention` | `Attention2dLayer` | `<32, 32, 16, 8, 8, 8, 32, 2, false, false>` |
| `cc_vit_attention` | `Attention2dLayer` | `<32, 32, 16, 8, 8, 8, 32, 2, true, true>` |
| `cc_vit_attention` | `Attention2dLayer` | `<32, 32, 16, 8, 8, 8, 32, 2, false, true>` |
| `cc_vit_attention` | `Attention2dLayer` | `<32, 32, 16, 8, 8, 8, 32, 2, true, false>` |
| `cc_vit_ffn_contract` | `Conv2d1x1Layer` | `<1024, 4096, 16, 8, 128, 32, 2, 2, 4, true, false, false, 128>` |
| `cc_vit_ffn_contract` | `Conv2d1x1Layer` | `<1024, 4096, 16, 8, 128, 32, 2, 2, 4, true, true, true, 128>` |
| `cc_vit_ffn_expand` | `Conv2d1x1Layer` | `<4096, 1024, 16, 8, 128, 32, 2, 2, 1, false, false, false, 128>` |
| `cc_vit_ffn_expand` | `Conv2d1x1Layer` | `<4096, 1024, 16, 8, 128, 32, 2, 2, 1, false, true, true, 128>` |
| `cc_vit_ffn_expand` | `Conv2d1x1Layer` | `<4096, 1024, 16, 8, 128, 32, 2, 2, 1, false, false, true, 128>` |
| `cc_vit_ffn_expand` | `Conv2d1x1Layer` | `<4096, 1024, 16, 8, 128, 32, 2, 2, 1, false, true, false, 128>` |
| `cc_vit_projection` | `Conv2d1x1Layer` | `<1024, 1024, 16, 8, 128, 32, 2, 2, 4, true, false, false, 32>` |
| `cc_vit_projection` | `Conv2d1x1Layer` | `<1024, 1024, 16, 8, 128, 32, 2, 2, 4, true, true, true, 32>` |
| `cc_vit_projection` | `Conv2d1x1Layer` | `<1024, 1024, 16, 8, 128, 32, 2, 2, 4, true, false, true, 32>` |
| `cc_vit_projection` | `Conv2d1x1Layer` | `<1024, 1024, 16, 8, 128, 32, 2, 2, 4, true, true, false, 32>` |
| `cc_vit_qkv` | `Conv2dQKVLayer` | `<32, 32, 1024, 16, 8, 32, 2, 2, 2, false, false, 128>` |
| `cc_vit_qkv` | `Conv2dQKVLayer` | `<32, 32, 1024, 16, 8, 32, 2, 2, 2, true, true, 128>` |
| `cc_vit_qkv` | `Conv2dQKVLayer` | `<32, 32, 1024, 16, 8, 32, 2, 2, 2, true, false, 128>` |
