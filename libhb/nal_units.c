/* nal_units.c
 *
 * Copyright (c) 2003-2025 HandBrake Team
 * Copyright (c) FFmpeg
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

size_t hb_nal_unit_payload_write_isomp4(uint8_t *buf,
                                        const uint8_t *payload,
                                        size_t payload_size,
                                        const int nal_type,
                                        const uint8_t nal_length_size)
{
    int i;
    uint8_t length[4]; // 4-byte length replaces Annex B start code prefix
    uint8_t header[2]; // 2-byte NAL header

    size_t nal_unit_size = payload_size + 2;

    if (buf != NULL)
    {
        for (i = 0; i < nal_length_size; i++)
        {
            length[i] = (nal_unit_size >> (8 * (nal_length_size - 1 - i))) & 0xff;
        }

        header[0] = nal_type << 1;
        header[1] = 1;

        memcpy(buf, &length[0], nal_length_size);
        buf += nal_length_size;
        memcpy(buf, &header[0], sizeof(header));
        buf += sizeof(header);
        memcpy(buf, payload, payload_size);
    }

    return nal_length_size + nal_unit_size;
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

uint8_t* hb_isomp4_find_next_nalu(const uint8_t *start, size_t *size, const uint8_t nal_length_size)
{
    uint8_t *nal = NULL;
    uint8_t *buf = (uint8_t*)start;
    uint8_t *end = (uint8_t*)start + *size;

    if (nal_length_size > 4)
    {
        return NULL;
    }

    while (end - buf > nal_length_size)
    {
        uint8_t length[4]; // 4-byte length replaces Annex B start code prefix
        size_t nal_unit_size = 0;

        memcpy(length, buf, nal_length_size);

        for (int i = 0; i < nal_length_size; i++)
        {
            nal_unit_size <<= 8;
            nal_unit_size |= length[i];
        }

        nal = buf;
        *size = nal_unit_size + nal_length_size;

        return nal;
    }

    return NULL;
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
                                  const uint8_t nal_length_size,
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

static int is_post_hevc_sei_nal_type(int nal_type)
{
    return nal_type != HB_HEVC_NAL_UNIT_PREFIX_SEI &&
           nal_type != HB_HEVC_NAL_UNIT_SPS &&
           nal_type != HB_HEVC_NAL_UNIT_PPS &&
           nal_type != HB_HEVC_NAL_UNIT_ACCESS_UNIT_DELIMITER;
}

/**
 * Copies the data inserting emulation prevention bytes as needed.
 * Existing data in the destination can be taken into account by providing
 * dst with a dst_offset > 0.
 *
 * @return The number of bytes copied on success. On failure, the negative of
 *         the number of bytes needed to copy src is returned.
 */
static int copy_emulation_prev(const uint8_t *src,
                               size_t         src_size,
                               uint8_t       *dst,
                               ssize_t        dst_offset,
                               size_t         dst_size)
{
    int zeros = 0;
    int wrote_bytes = 0;
    uint8_t *dst_end = dst ? dst + dst_size : NULL;
    const uint8_t* src_end = src + src_size;
    int start_at = dst_offset > 2 ? dst_offset - 2 : 0;

    for (int i = start_at; i < dst_offset && i < dst_size; i++)
    {
        if (!dst[i])
        {
            zeros++;
        }
        else
        {
            zeros = 0;
        }
    }

    if (dst)
    {
        dst += dst_offset;
    }
    for (; src < src_end;)
    {
        if (zeros == 2)
        {
            int insert_ep3_byte = *src <= 3;
            if (insert_ep3_byte)
            {
                if (dst < dst_end)
                {
                    *dst = 3;
                }
                if (dst)
                {
                    dst++;
                }
                wrote_bytes++;
            }

            zeros = 0;
        }

        if (dst < dst_end)
        {
            *dst = *src;
        }

        if (!*src)
        {
            zeros++;
        }
        else
        {
            zeros = 0;
        }

        src++;
        if (dst)
        {
            dst++;
        }
        wrote_bytes++;
    }

    if (!dst)
    {
        return -wrote_bytes;
    }

    return wrote_bytes;
}

/**
 * Returns a sufficient number of bytes to contain the sei data.
 * It may be greater than the minimum required.
 */
static int get_sei_msg_bytes(const uint8_t *sei_data, const size_t sei_size, int type)
{
    int copied_size;
    if (sei_size == 0)
    {
        return 0;
    }

    copied_size = -copy_emulation_prev(sei_data,
                                       sei_size,
                                       NULL,
                                       0,
                                       0);

    if ((sei_size % 255) == 0) //may result in an extra byte
    {
        copied_size++;
    }

    return copied_size + sei_size / 255 + 1 + type / 255 + 1;
}

size_t hb_sei_unit_write_isomp4(const uint8_t *sei,
                                const size_t   sei_size,
                                int            sei_type,
                                uint8_t       *dst,
                                size_t         dst_size,
                                const uint8_t  nal_length_size)
{
    size_t remaining_sei_size = sei_size;
    size_t remaining_dst_size = dst_size;
    int sei_header_bytes;
    int bytes_written = 0;
    ssize_t offset;

    if (!remaining_dst_size)
    {
        return -1;
    }

    size_t sei_nalu_size = dst_size - nal_length_size;

    uint8_t length[4]; // up to 4-byte length replaces Annex B start code prefix
    uint8_t header[2]; // 2-byte NAL header

    for (int i = 0; i < nal_length_size; i++)
    {
        length[i] = (sei_nalu_size >> (8 * (nal_length_size - 1 - i))) & 0xff;
    }
    memcpy(dst, &length[0], nal_length_size);
    dst += nal_length_size;

    // NAL Header
    header[0] = HB_HEVC_NAL_UNIT_PREFIX_SEI << 1;
    header[1] = 1;

    memcpy(dst, &header[0], sizeof(header));
    dst += sizeof(header);

    remaining_dst_size -= nal_length_size + sizeof(header);

    uint8_t *sei_start = dst;

    while (sei_type && remaining_dst_size != 0)
    {
        int sei_byte = sei_type > 255 ? 255 : sei_type;
        *dst = sei_byte;

        sei_type -= sei_byte;
        dst++;
        remaining_dst_size--;
    }

    if (!dst_size)
    {
        return -1;
    }

    while (remaining_sei_size && remaining_dst_size != 0)
    {
        int size_byte = remaining_sei_size > 255 ? 255 : remaining_sei_size;
        *dst = size_byte;

        remaining_sei_size -= size_byte;
        dst++;
        remaining_dst_size--;
    }

    if (remaining_dst_size < sei_size)
    {
        return -1;
    }

    sei_header_bytes = dst - sei_start;

    offset = sei_header_bytes;
    bytes_written = copy_emulation_prev(sei,
                                        sei_size,
                                        sei_start,
                                        offset,
                                        dst_size - nal_length_size - sizeof(header) - 1);
    if (bytes_written < 0)
    {
        return -1;
    }

    remaining_dst_size -= bytes_written;

    if (remaining_dst_size < 1)
    {
        return -1;
    }

    // rbsp_stop_one_bit
    dst+= bytes_written;
    *dst = 0x80;

    return nal_length_size + sizeof(header) + sei_header_bytes + bytes_written + 1;
}

hb_buffer_t * hb_isomp4_hevc_nal_bitstream_insert_payloads(const uint8_t *data,
                                                           const size_t size,
                                                           const hb_sei_t *seis,
                                                           const size_t sei_count,
                                                           const hb_nal_t *nals,
                                                           const size_t nal_count,
                                                           const uint8_t nal_length_size)
{
    hb_buffer_t *out;
    const uint8_t *buf, *end;
    uint8_t *out_data;
    size_t out_size = 0, buf_size;

    size_t sei_nalu_size[4];
    uint8_t sei_written[4];

    if ((seis == NULL || sei_count == 0) &&
        (nals == NULL || nal_count == 0))
    {
        return NULL;
    }

    if ((seis == NULL && sei_count > 0) ||
        (nals == NULL && nal_count > 0))
    {
        return NULL;
    }

    for (int i = 0; i < sei_count; i++)
    {
        size_t msg_size = get_sei_msg_bytes(seis[i].payload, seis[i].payload_size, seis[i].type);
        sei_nalu_size[i] = nal_length_size + 2 + msg_size + 1;
        sei_written[i] = 0;

        out_size += sei_nalu_size[i];
    }

    for (int i = 0; i < nal_count; i++)
    {
        size_t nalu_size = nal_length_size + 2 + nals[i].payload_size;
        out_size += nalu_size;
    }

    out_size += size;

    out = hb_buffer_init(out_size);
    if (out == NULL)
    {
        hb_error("hb_nal_bitstream_insert_sei: hb_buffer_init failed");
        return NULL;
    }

    buf_size = size;
    buf      = data;
    end      = data + size;
    out_data = out->data;

    while ((buf = hb_isomp4_find_next_nalu(buf, &buf_size, nal_length_size)) != NULL)
    {
        uint8_t nal_type = buf[nal_length_size] >> 1;

        for (int i = 0; i < sei_count; i++)
        {
            if (!sei_written[i] && is_post_hevc_sei_nal_type(nal_type))
            {
                out_data += hb_sei_unit_write_isomp4(seis[i].payload, seis[i].payload_size, seis[i].type,
                                                     out_data, sei_nalu_size[i], nal_length_size);
                sei_written[i] = 1;
            }
        }

        memcpy(out_data, buf, buf_size);
        out_data += buf_size;
        buf      += buf_size;
        buf_size  = end - buf;
    }

    for (int i = 0; i < nal_count; i++)
    {
        // Append the DOVI RPU payload at the end
        out_data += hb_nal_unit_payload_write_isomp4(out_data, nals[i].payload, nals[i].payload_size,
                                                     nals[i].type, nal_length_size);
    }

    return out;
}
