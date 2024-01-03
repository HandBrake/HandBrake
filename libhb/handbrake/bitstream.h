/* bitstream.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_BITSTREAM_H
#define HANDBRAKE_BITSTREAM_H

#include <stdint.h>

typedef struct
{
    uint8_t    *buf;
    uint32_t    pos;
    uint32_t    buf_size;
} hb_bitstream_t;

void hb_bitstream_init(hb_bitstream_t *bs,
                       uint8_t *buf,
                       uint32_t bufsize,
                       int clear);

void hb_bitstream_put_bytes(hb_bitstream_t *bs,
                            uint8_t *bytes,
                            uint32_t num_bytes);

void hb_bitstream_put_bits(hb_bitstream_t *bs,
                           uint32_t bits,
                           uint32_t num_bits);

uint32_t hb_bitstream_peak_bits(hb_bitstream_t *bs,
                                uint32_t num_bits);

uint32_t hb_bitstream_get_bits(hb_bitstream_t *bs,
                               uint32_t num_bits);

void hb_bitstream_skip_bytes(hb_bitstream_t *bs,
                             uint32_t num_bytes);

void hb_bitstream_skip_bits(hb_bitstream_t *bs,
                            uint32_t num_bits);

uint32_t hb_bitstream_get_bit_position(hb_bitstream_t *bs);

void hb_bitstream_set_bit_position(hb_bitstream_t *bs,
                                   uint32_t bitPos);

uint8_t * hb_bitstream_get_buffer(hb_bitstream_t *bs);

uint32_t hb_bitstream_get_count_of_bytes(hb_bitstream_t *bs);

uint32_t hb_bitstream_get_count_of_bits(hb_bitstream_t *bs);

uint32_t hb_bitstream_get_count_of_used_bytes(hb_bitstream_t *bs);

uint32_t hb_bitstream_get_remaining_bits(hb_bitstream_t *bs);

#endif // HANDBRAKE_BITSTREAM_H
