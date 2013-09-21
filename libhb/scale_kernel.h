/* scale_kernel.h

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html

   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>


 */
 
#ifndef _H_SCALE_KERNEL_H
#define _H_SCALE_KERNEL_H
#ifdef USE_OPENCL
void av_scale_frame(ScaleContext *c, void *dst, void *src, int *srcStride, int *dstStride, int *should_dither);
#endif
#endif
