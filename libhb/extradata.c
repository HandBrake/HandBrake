/* extradata.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/extradata.h"
#include "handbrake/bitstream.h"
#include "libavutil/intreadwrite.h"
#include <ogg/ogg.h>

int hb_set_extradata(hb_data_t **extradata, const uint8_t *bytes, size_t length)
{
    hb_data_close(extradata);

    if (length > 0)
    {
        *extradata = hb_data_init(length);
        if (*extradata == NULL)
        {
            hb_error("extradata: malloc failure");
            return 1;
        }
        memcpy((*extradata)->bytes, bytes, length);
    }
    return 0;
}

int hb_set_text_extradata(hb_data_t **extradata, const uint8_t *bytes, size_t length)
{
    hb_data_close(extradata);

    if (length > 0)
    {
        *extradata = hb_data_init(length + 1);
        if (*extradata == NULL)
        {
            hb_error("extradata: malloc failure");
            return 1;
        }
        memcpy((*extradata)->bytes, bytes, length);
        (*extradata)->bytes[length] = 0;
    }
    return 0;
}

int hb_set_ssa_extradata(hb_data_t **extradata, const char *font, int fs, int w, int h)
{
    hb_data_close(extradata);

    char *header = NULL;
    float shadow_size = fs / 36.0;
    float outline_size = fs / 30.0;

    char *shadow_size_string = hb_strdup_printf("%.2f", shadow_size);
    hb_str_from_locale(shadow_size_string);

    char *outline_size_string = hb_strdup_printf("%.2f", outline_size);
    hb_str_from_locale(outline_size_string);

    if (shadow_size_string == NULL || outline_size_string == NULL)
    {
        goto fail;
    }

    // SRT subtitles are represented internally as SSA
    // Create an SSA header
    const char * ssa_header =
        "[Script Info]\r\n"
        "ScriptType: v4.00+\r\n"
        "Collisions: Normal\r\n"
        "PlayResX: %d\r\n"
        "PlayResY: %d\r\n"
        "Timer: 100.0\r\n"
        "WrapStyle: 0\r\n"
        "ScaledBorderAndShadow: yes\r\n"
        "\r\n"
        "[V4+ Styles]\r\n"
        "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\r\n"
        "Style: Default,%s,%d,&H00FFFFFF,&H00FFFFFF,&H000F0F0F,&H000F0F0F,0,0,0,0,100,100,0,0.00,1,%s,%s,2,20,20,20,0\r\n";

    header = hb_strdup_printf(ssa_header, w, h, font, fs, outline_size_string, shadow_size_string);
    if (header == NULL)
    {
        goto fail;
    }

    size_t size = strlen(header) + 1;
    *extradata = hb_data_init(size);
    if (*extradata == NULL)
    {
        goto fail;
    }

    memcpy((*extradata)->bytes, header, size);
    free(header);
    free(shadow_size_string);
    free(outline_size_string);

    return 0;

fail:
    hb_error("ssa extradata: malloc failure");
    free(header);
    free(shadow_size_string);
    free(outline_size_string);

    return 1;
}

int hb_set_h264_extradata(hb_data_t **extradata, uint8_t *sps, size_t sps_length, uint8_t *pps, size_t pps_length)
{
    hb_data_close(extradata);

    /* Taken from x264 muxers.c */
    size_t length = 5 + 1 + 2 + sps_length + 1 + 2 + pps_length;
    *extradata = hb_data_init(length);
    if (*extradata == NULL)
    {
        hb_error("H.264 extradata: malloc failure");
        return 1;
    }

    uint8_t *data = (*extradata)->bytes;

    data[0] = 1;
    data[1] = sps[1]; /* AVCProfileIndication */
    data[2] = sps[2]; /* profile_compat */
    data[3] = sps[3]; /* AVCLevelIndication */
    data[4] = 0xff; // nalu size length is four bytes
    data[5] = 0xe1; // one sps

    data[6] = sps_length >> 8;
    data[7] = sps_length;

    memcpy(data + 8, sps, sps_length);

    data[8  + sps_length] = 1; // one pps
    data[9  + sps_length] = pps_length >> 8;
    data[10 + sps_length] = pps_length;

    memcpy(data + 11 + sps_length, pps, pps_length);

    return 0;
}

int hb_set_xiph_extradata(hb_data_t **extradata, uint8_t headers[3][HB_CONFIG_MAX_SIZE])
{
    hb_data_close(extradata);

    int size = 0;
    ogg_packet *ogg_headers[3];

    for (int ii = 0; ii < 3; ii++)
    {
        ogg_headers[ii] = (ogg_packet *)headers[ii];
        size += ogg_headers[ii]->bytes + 2;
    }

    *extradata = hb_data_init(size);
    if (*extradata == NULL)
    {
        hb_error("xiph extradata: malloc failure");
        return 1;
    }

    uint8_t *data = (*extradata)->bytes;
    size = 0;

    for (int ii = 0; ii < 3; ii++)
    {
        AV_WB16(data + size, ogg_headers[ii]->bytes);
        size += 2;
        memcpy(data + size, ogg_headers[ii]->packet,
                               ogg_headers[ii]->bytes);
        size += ogg_headers[ii]->bytes;
    }

    return 0;
}

static int64_t get_uvlc(hb_bitstream_t *bs)
{
    int leadingZeros = 0;

    while (1)
    {
        int done = hb_bitstream_get_bits(bs, 1);
        if (done)
        {
            break;
        }
        leadingZeros += 1;
    }
    if (leadingZeros >= 32)
    {
        return ((int64_t)1 << 32) - 1;
    }

    return hb_bitstream_get_bits(bs, leadingZeros);
}

static int get_leb128(hb_bitstream_t *bs)
{
    int value = 0;
    for (int i = 0; i < 8; i++)
    {
        int leb128_byte = hb_bitstream_get_bits(bs, 8);
        value |= ((leb128_byte & 0x7f) << (i * 7));
        if (!(leb128_byte & 0x80))
        {
            break;
        }
    }
    return value;
}

int hb_parse_av1_extradata(hb_data_t *extradata, int *level_idx, int *high_tier)
{
    hb_bitstream_t bs;
    int obu_type, obu_extension_flag, obu_has_size_field, obu_size = 0;
    int seq_level_idx = 0, seq_tier = 0;

    hb_bitstream_init(&bs, extradata->bytes, extradata->size, 0);

    hb_bitstream_skip_bits(&bs, 1);  // obu_forbidden_bit
    obu_type = hb_bitstream_get_bits(&bs, 4);
    obu_extension_flag = hb_bitstream_get_bits(&bs, 1);
    obu_has_size_field = hb_bitstream_get_bits(&bs, 1);
    hb_bitstream_skip_bits(&bs, 1);  // obu_reserved_1bit

    if (obu_type != 1)
    {
        goto fail;
    }

    if (obu_extension_flag)
    {
        hb_bitstream_skip_bits(&bs, 3);  // temporal_id
        hb_bitstream_skip_bits(&bs, 2);  // spatial_id
        hb_bitstream_skip_bits(&bs, 3);  // extension_header_reserved_3bits
    }

    if (obu_has_size_field)
    {
        obu_size = get_leb128(&bs);
    }
    else
    {
        obu_size = extradata->size - 1 - obu_extension_flag;
    }

    if (obu_size == 0)
    {
        goto fail;
    }

    hb_bitstream_skip_bits(&bs, 3);  // seq_profile
    hb_bitstream_skip_bits(&bs, 1);  // still_picture
    int reduced_still_picture_header = hb_bitstream_get_bits(&bs, 1);
    if (reduced_still_picture_header)
    {
        seq_level_idx = hb_bitstream_get_bits(&bs, 5);
        seq_tier = 0;
    }
    else
    {
        int timing_info_present_flag = hb_bitstream_get_bits(&bs, 1);
        if (timing_info_present_flag)
        {
            hb_bitstream_skip_bits(&bs, 32);  // num_units_in_display_tick
            hb_bitstream_skip_bits(&bs, 32);  // time_scale
            int equal_picture_interval = hb_bitstream_get_bits(&bs, 1);  // time_scale
            if (equal_picture_interval)
            {
                get_uvlc(&bs); // num_ticks_per_picture_minus_1
            }

            int decoder_model_info_present_flag = hb_bitstream_get_bits(&bs, 1);
            if (decoder_model_info_present_flag)
            {
                hb_bitstream_skip_bits(&bs, 5);  // buffer_delay_length_minus_1
                hb_bitstream_skip_bits(&bs, 32); // num_units_in_decoding_tick
                hb_bitstream_skip_bits(&bs, 5);  // buffer_removal_time_length_minus_1
                hb_bitstream_skip_bits(&bs, 3);  // frame_presentation_time_length_minus_1
            }
        }

        hb_bitstream_skip_bits(&bs, 1); // initial_display_delay_present_flag
        int operating_points_cnt_minus_1 = hb_bitstream_get_bits(&bs, 5);
        for (int i = 0; i <= operating_points_cnt_minus_1; i++)
        {
            hb_bitstream_skip_bits(&bs, 12);
            seq_level_idx = hb_bitstream_get_bits(&bs, 5);
            if (seq_level_idx > 7)
            {
                seq_tier = hb_bitstream_get_bits(&bs, 1);
            }
        }
    }

    *high_tier = seq_tier;
    *level_idx = seq_level_idx;
    return 0;

fail:
    *high_tier = 0;
    *level_idx = 0;
    return 1;
}

int hb_parse_h265_extradata(hb_data_t *extradata, int *level_idc, int *high_tier)
{
    hb_bitstream_t bs;

    if (extradata->size < 13)
    {
        *level_idc = 0;
        *high_tier = 0;
        return 1;
    }

    hb_bitstream_init(&bs, extradata->bytes, extradata->size, 0);

    hb_bitstream_skip_bits(&bs, 8);  // configurationVersion
    hb_bitstream_skip_bits(&bs, 2);  // general_profile_space
    int general_tier_flag = hb_bitstream_get_bits(&bs, 1);
    hb_bitstream_skip_bits(&bs, 5);  // general_profile_idc
    hb_bitstream_skip_bits(&bs, 32); // general_profile_compatibility_flags

    hb_bitstream_skip_bits(&bs, 32); // << 16 general_constraint_indicator_flags
    hb_bitstream_skip_bits(&bs, 16);
    int general_level_idc = hb_bitstream_get_bits(&bs, 8);

    *high_tier = general_tier_flag;
    *level_idc = general_level_idc / 3;
    return 0;
}
