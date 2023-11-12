/* nlmeans.h

   Copyright (c) 2013 Dirk Farin
   Copyright (c) 2003-2023 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_NLMEANS_H
#define HANDBRAKE_NLMEANS_H

typedef struct
{
    void (*build_integral)(uint32_t *integral,
                           int       integral_stride,
                     const void  *src,
                     const void  *src_pre,
                     const void  *compare,
                     const void  *compare_pre,
                           int    w,
                           int    border,
                           int    dst_w,
                           int    dst_h,
                           int    dx,
                           int    dy,
                           int    n);
} NLMeansFunctions;

void nlmeans_init_x86(NLMeansFunctions *functions);

#endif // HANDBRAKE_NLMEANS_H
