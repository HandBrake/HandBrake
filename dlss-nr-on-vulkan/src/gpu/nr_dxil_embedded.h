/*
 * nr_dxil_embedded — the DXIL modules compiled into libd3dmx.
 *
 * With `NR_BUILD_D3D12` the CMake build runs dxc on each HLSL kernel under `src/gpu/d3d12/`,
 * bin2c on each `.dxil` it writes, and compiles the lot, with this table, into libd3dmx (and
 * into libdlssnr when the archive is built on Direct3D 12). The rows reuse the SPIR-V table's
 * row type: the name is the file dxc wrote, e.g. "resident.dxil".
 */
#ifndef NR_DXIL_EMBEDDED_H
#define NR_DXIL_EMBEDDED_H
#include "nr_shaders_embedded.h"

#ifdef NR_EMBEDDED_DXIL
extern const struct nr_embedded_shader nr_embedded_dxil[];
extern const size_t nr_embedded_dxil_count;
#endif

#endif
