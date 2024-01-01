/* hdr10plus.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_HDR_10_PLUS_H
#define HANDBRAKE_HDR_10_PLUS_H

#include <stdint.h>
#include "libavutil/hdr_dynamic_metadata.h"

void hb_dynamic_hdr10_plus_to_itu_t_t35(const AVDynamicHDRPlus *s, uint8_t **buf_p, uint32_t *size);

#endif // HANDBRAKE_HDR_10_PLUS_H
