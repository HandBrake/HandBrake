/*
 * nr_shaders_embedded — every SPIR-V module compiled into the runtime.
 *
 * With `NR_EMBED_SHADERS` (the default) the CMake build runs bin2c on each `.spv` that
 * glslangValidator writes and compiles the lot, with this table, into libxmx and
 * gemm_runner. A shader is then named rather than located: `xmx_init("gemm_coopmat.spv")`
 * — a bare name, no directory — takes the compiled-in module, a path with a directory in
 * it still opens that file (so the `XMX_*_SPV` overrides keep working for experiments),
 * and a path whose file is missing falls back to the embedded module of the same name.
 * `xmx_embedded_shader(name)` says whether the runtime has one, by size.
 *
 * A Makefile build defines none of this and loads files as it always did.
 */
#ifndef NR_SHADERS_EMBEDDED_H
#define NR_SHADERS_EMBEDDED_H
#include <stddef.h>

struct nr_embedded_shader {
    const char *name;               /* the file name glslang wrote, e.g. "resident.spv" */
    const unsigned char *data;      /* the SPIR-V words, from bin2c */
    size_t size;
};

#ifdef NR_EMBEDDED_SHADERS
extern const struct nr_embedded_shader nr_embedded_shaders[];
extern const size_t nr_embedded_shader_count;
#endif

#endif
