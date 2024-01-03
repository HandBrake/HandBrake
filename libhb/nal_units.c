/* nal_units.c
 *
 * Copyright (c) 2003-2024 HandBrake Team
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License v2.
 * For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdint.h>
#include <string.h>

#include "handbrake/common.h"
#include "handbrake/nal_units.h"

static const uint8_t hb_annexb_startcode[] = { 0x00, 0x00, 0x00, 0x01, };

size_t hb_nal_unit_write_annexb(uint8_t *buf,
                                const uint8_t *nal_unit,
                                const size_t nal_unit_size)
{
    if (buf != NULL)
    {
        memcpy(buf, hb_annexb_startcode, sizeof(hb_annexb_startcode));
        memcpy(buf + sizeof(hb_annexb_startcode), nal_unit, nal_unit_size);
    }

    return sizeof(hb_annexb_startcode) + nal_unit_size;
}

size_t hb_nal_unit_write_isomp4(uint8_t *buf,
                                const uint8_t *nal_unit,
                                const size_t nal_unit_size)
{
    int i;
    uint8_t length[4]; // 4-byte length replaces Annex B start code prefix

    if (buf != NULL)
    {
        for (i = 0; i < sizeof(length); i++)
        {
            length[i] = (nal_unit_size >> (8 * (sizeof(length) - 1 - i))) & 0xff;
        }

        memcpy(buf, &length[0], sizeof(length));
        memcpy(buf + sizeof(length), nal_unit, nal_unit_size);
    }

    return sizeof(length) + nal_unit_size;
}

uint8_t* hb_annexb_find_next_nalu(const uint8_t *start, size_t *size)
{
    uint8_t *nal = NULL;
    uint8_t *buf = (uint8_t*)start;
    uint8_t *end = (uint8_t*)start + *size;

    /* Look for an Annex B start code prefix (3-byte sequence == 1) */
    while (end - buf > 3)
    {
        if (!buf[0] && !buf[1] && buf[2] == 1)
        {
            nal = (buf += 3); // NAL unit begins after start code
            break;
        }
        buf++;
    }

    if (nal == NULL)
    {
        *size = 0;
        return NULL;
    }

    /*
     * Start code prefix found, look for the next one to determine the size
     *
     * A 4-byte sequence == 1 is also a start code, so check for a 3-byte
     * sequence == 0 too (start code emulation prevention will prevent such a
     * sequence from occurring outside of a start code prefix)
     */
    while (end - buf > 3)
    {
        if (!buf[0] && !buf[1] && (!buf[2] || buf[2] == 1))
        {
            end = buf;
            break;
        }
        buf++;
    }

    *size = end - nal;
    return  nal;
}

hb_buffer_t* hb_nal_bitstream_annexb_to_mp4(const uint8_t *data,
                                            const size_t size)
{
    hb_buffer_t *out;
    uint8_t *buf, *end;
    size_t out_size, buf_size;

    out_size = 0;
    buf_size = size;
    buf      = (uint8_t*)data;
    end      = (uint8_t*)data + size;

    while ((buf = hb_annexb_find_next_nalu(buf, &buf_size)) != NULL)
    {
        out_size += hb_nal_unit_write_isomp4(NULL, buf, buf_size);
        buf_size  = end - buf;
    }

    out = hb_buffer_init(out_size);
    if (out == NULL)
    {
        hb_error("hb_nal_bitstream_annexb_to_mp4: hb_buffer_init failed");
        return NULL;
    }

    out_size = 0;
    buf_size = size;
    buf      = (uint8_t*)data;
    end      = (uint8_t*)data + size;

    while ((buf = hb_annexb_find_next_nalu(buf, &buf_size)) != NULL)
    {
        out_size += hb_nal_unit_write_isomp4(out->data + out_size, buf, buf_size);
        buf_size  = end - buf;
    }

    return out;
}

static size_t mp4_nal_unit_length(const uint8_t *data,
                                  const size_t nal_length_size,
                                  size_t *nal_unit_length)
{
    uint8_t i;

    /* In MP4, NAL units are preceded by a 2-4 byte length field */
    for (i = 0, *nal_unit_length = 0; i < nal_length_size; i++)
    {
        *nal_unit_length |= data[i] << (8 * (nal_length_size - 1 - i));
    }

    return nal_length_size;
}

hb_buffer_t* hb_nal_bitstream_mp4_to_annexb(const uint8_t *data,
                                            const size_t size,
                                            const uint8_t nal_length_size)
{
    hb_buffer_t *out;
    uint8_t *buf, *end;
    size_t out_size, nal_size;

    out_size = 0;
    buf      = (uint8_t*)data;
    end      = (uint8_t*)data + size;

    while (end - buf > nal_length_size)
    {
        buf += mp4_nal_unit_length(buf, nal_length_size, &nal_size);
        if (end - buf < nal_size)
        {
            hb_log("hb_nal_bitstream_mp4_to_annexb: truncated bitstream"
                   " (remaining: %lu, expected: %lu)", end - buf, nal_size);
            return NULL;
        }

        out_size += hb_nal_unit_write_annexb(NULL, buf, nal_size);
        buf      += nal_size;
    }

    out = hb_buffer_init(out_size);
    if (out == NULL)
    {
        hb_error("hb_nal_bitstream_mp4_to_annexb: hb_buffer_init failed");
        return NULL;
    }

    out_size = 0;
    buf      = (uint8_t*)data;
    end      = (uint8_t*)data + size;

    while (end - buf > nal_length_size)
    {
        buf      += mp4_nal_unit_length(buf, nal_length_size, &nal_size);
        out_size += hb_nal_unit_write_annexb(out->data + out_size, buf, nal_size);
        buf      += nal_size;
    }

    return out;
}
