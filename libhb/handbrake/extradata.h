/* extradata.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_EXTRADATA_H
#define HANDBRAKE_EXTRADATA_H

#ifdef __LIBHB__

#include "handbrake/handbrake.h"

int hb_set_extradata(hb_data_t **extradata, const uint8_t *bytes, size_t length);

int hb_set_h264_extradata(hb_data_t **extradata, uint8_t *sps, size_t sps_length, uint8_t *pps, size_t pps_length);
int hb_set_xiph_extradata(hb_data_t **extradata, uint8_t headers[3][HB_CONFIG_MAX_SIZE]);

int hb_set_text_extradata(hb_data_t **extradata, const uint8_t *bytes, size_t length);
int hb_set_ssa_extradata(hb_data_t **extradata, const char *font, int fs, int w, int h);

#endif

#endif /* HANDBRAKE_TASKSET_H */
