#ifndef _H_SCALE_KERNEL_H
#define _H_SCALE_KERNEL_H
#ifdef USE_OPENCL
void av_scale_frame(ScaleContext *c, void *dst, void *src, int *srcStride, int *dstStride, int *should_dither);
#endif
#endif
