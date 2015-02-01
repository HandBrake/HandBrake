/* oclnv12toyuv.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
   
   Authors: Peng Gao <peng@multicorewareinc.com> <http://www.multicorewareinc.com/>
            Li   Cao <li@multicorewareinc.com> <http://www.multicorewareinc.com/>

 */

#ifndef HB_OCLNV12TOYUV_H
#define HB_OCLNV12TOYUV_H

#include "common.h"
#include "extras/cl.h"
#include "openclwrapper.h"

/*
 * nv12 to yuv interface
 * bufi is input frame of nv12, w is input frame width, h is input frame height
 */
int hb_ocl_nv12toyuv(uint8_t *bufi[], int p, int w, int h, int *crop, hb_va_dxva2_t *dxva2, int decomb, int detelecine);

#endif // HB_OCLNV12TOYUV_H
