/* bitstream.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <string.h>
#include "handbrake/bitstream.h"

void hb_bitstream_init(hb_bitstream_t *bs,
                       uint8_t *buf,
                       uint32_t buf_size,
                       int clear)
{
    bs->pos = 0;
    bs->buf = buf;
    bs->buf_size = buf_size << 3;
    if (clear)
    {
        memset(bs->buf, 0, buf_size);
    }
}

void hb_bitstream_put_bytes(hb_bitstream_t *bs,
                            uint8_t *bytes,
                            uint32_t num_bytes)
{
    uint32_t num_bits = num_bytes << 3;

    if (num_bits + bs->pos > bs->buf_size)
    {
        return;
    }

    if ((bs->pos & 7) == 0)
    {
        memcpy(&bs->buf[bs->pos >> 3], bytes, num_bytes);
        bs->pos += num_bits;
    }
    else
    {
        for (uint32_t i = 0; i < num_bytes; i++)
        {
            hb_bitstream_put_bits(bs, bytes[i], 8);
        }
    }
}

void hb_bitstream_put_bits(hb_bitstream_t *bs,
                           uint32_t bits,
                           uint32_t num_bits)
{
    if (num_bits + bs->pos > bs->buf_size)
    {
        return;
    }
    if (num_bits > 32) {
        return;
    }

    for (int8_t i = num_bits - 1; i >= 0; i--)
    {
        bs->buf[bs->pos >> 3] |= ((bits >> i) & 1) << (7 - (bs->pos & 7));
        bs->pos++;
    }

}

uint32_t hb_bitstream_peak_bits(hb_bitstream_t *bs,
                                uint32_t num_bits)
{
    if (num_bits + bs->pos > bs->buf_size)
    {
        return 0;
    }
    if (num_bits > 32) {
        return 0;
    }

    uint32_t value = 0;
    uint32_t pos = bs->pos;

    for (uint8_t i = 0; i < num_bits; i++)
    {
        value <<= 1;
        value |= (bs->buf[pos >> 3] >> (7 - (pos & 7))) & 1;
        pos++;
    }

    return value;
}

uint32_t hb_bitstream_get_bits(hb_bitstream_t *bs,
                               uint32_t num_bits)
{
    if (num_bits + bs->pos > bs->buf_size)
    {
        return 0;
    }
    if (num_bits > 32)
    {
        return 0;
    }

    uint32_t value = 0;

    for (uint8_t i = 0; i < num_bits; i++)
    {
        value <<= 1;
        value |= (bs->buf[bs->pos >> 3] >> (7 - (bs->pos & 7))) & 1;
        bs->pos++;
    }

    return value;
}

void hb_bitstream_skip_bytes(hb_bitstream_t *bs,
                             uint32_t num_bytes)
{
    hb_bitstream_skip_bits(bs, num_bytes << 3);
}

void hb_bitstream_skip_bits(hb_bitstream_t *bs,
                            uint32_t num_bits)
{
    hb_bitstream_set_bit_position(bs, hb_bitstream_get_bit_position(bs) + num_bits);
}

uint32_t hb_bitstream_get_bit_position(hb_bitstream_t *bs)
{
    return bs->pos;
}

void hb_bitstream_set_bit_position(hb_bitstream_t *bs,
                                   uint32_t pos)
{
    if (pos > bs->buf_size)
    {
        return;
    }
    bs->pos = pos;
}

uint8_t * hb_bitstream_get_buffer(hb_bitstream_t *bs)
{
    return bs->buf;
}

uint32_t hb_bitstream_get_count_of_bytes(hb_bitstream_t *bs)
{
    return (hb_bitstream_get_count_of_bits(bs) + 7) / 8;
}

uint32_t hb_bitstream_get_count_of_bits(hb_bitstream_t *bs)
{
    return bs->buf_size;
}

uint32_t hb_bitstream_get_count_of_used_bytes(hb_bitstream_t *bs)
{
    return (bs->pos + 7) / 8;
}

uint32_t hb_bitstream_get_remaining_bits(hb_bitstream_t *bs)
{
    return bs->buf_size - bs->pos;
}
