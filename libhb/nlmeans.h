/* nlmeans.h

   Copyright (c) 2013 Dirk Farin
   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

typedef struct
{
    void (*build_integral)(uint32_t *integral,
                           int       integral_stride,
                     const uint8_t  *src,
                     const uint8_t  *src_pre,
                     const uint8_t  *compare,
                     const uint8_t  *compare_pre,
                           int       w,
                           int       border,
                           int       dst_w,
                           int       dst_h,
                           int       dx,
                           int       dy);
} NLMeansFunctions;

void nlmeans_init_x86(NLMeansFunctions *functions);
