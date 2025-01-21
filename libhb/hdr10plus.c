/* hdr10plus.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <string.h>
#include "handbrake/bitstream.h"
#include "libavutil/hdr_dynamic_metadata.h"

void hb_dynamic_hdr10_plus_to_itu_t_t35(const AVDynamicHDRPlus *s, uint8_t **buf_p, uint32_t *size)
{
    const uint8_t countryCode = 0xB5;
    const uint16_t terminalProviderCode = 0x003C;
    const uint16_t terminalProviderOrientedCode = 0x0001;
    const uint8_t applicationIdentifier = 4;

    uint8_t *buf = av_mallocz(2048);
    hb_bitstream_t bs;

    hb_bitstream_init(&bs, buf, 2048, 0);

    hb_bitstream_put_bits(&bs, countryCode, 8);
    hb_bitstream_put_bits(&bs, terminalProviderCode, 16);
    hb_bitstream_put_bits(&bs, terminalProviderOrientedCode, 16);

    hb_bitstream_put_bits(&bs, applicationIdentifier, 8);
    hb_bitstream_put_bits(&bs, s->application_version, 8);
    hb_bitstream_put_bits(&bs, s->num_windows, 2);

    for (int w = 1; w < s->num_windows; w++)
    {
        const AVHDRPlusColorTransformParams *params = &s->params[w];

        hb_bitstream_put_bits(&bs, params->window_upper_left_corner_x.num, 16);
        hb_bitstream_put_bits(&bs, params->window_upper_left_corner_x.num, 16);
        hb_bitstream_put_bits(&bs, params->window_upper_left_corner_x.num, 16);
        hb_bitstream_put_bits(&bs, params->window_upper_left_corner_x.num, 16);

        hb_bitstream_put_bits(&bs, params->center_of_ellipse_x, 16);
        hb_bitstream_put_bits(&bs, params->center_of_ellipse_y, 16);
        hb_bitstream_put_bits(&bs, params->rotation_angle, 8);
        hb_bitstream_put_bits(&bs, params->semimajor_axis_internal_ellipse, 16);
        hb_bitstream_put_bits(&bs, params->semimajor_axis_external_ellipse, 16);
        hb_bitstream_put_bits(&bs, params->semiminor_axis_external_ellipse, 16);
        hb_bitstream_put_bits(&bs, params->overlap_process_option, 1);
    }

    hb_bitstream_put_bits(&bs, s->targeted_system_display_maximum_luminance.num, 27);
    hb_bitstream_put_bits(&bs, s->targeted_system_display_actual_peak_luminance_flag, 1);

    if (s->targeted_system_display_actual_peak_luminance_flag)
    {
        hb_bitstream_put_bits(&bs, s->num_rows_targeted_system_display_actual_peak_luminance, 5);
        hb_bitstream_put_bits(&bs, s->num_cols_targeted_system_display_actual_peak_luminance, 5);

        for (int i = 0; i < s->num_rows_targeted_system_display_actual_peak_luminance; i++)
        {
            for (int j = 0; j < s->num_cols_targeted_system_display_actual_peak_luminance; j++)
            {
                hb_bitstream_put_bits(&bs, s->targeted_system_display_actual_peak_luminance[i][j].num, 4);
            }
        }
    }

    for (int w = 0; w < s->num_windows; w++)
    {
        const AVHDRPlusColorTransformParams *params = &s->params[w];

        for (int i = 0; i < 3; i++)
        {
            hb_bitstream_put_bits(&bs, params->maxscl[i].num, 17);
        }
        hb_bitstream_put_bits(&bs, params->average_maxrgb.num, 17);
        hb_bitstream_put_bits(&bs, params->num_distribution_maxrgb_percentiles, 4);

        for (int i = 0; i < params->num_distribution_maxrgb_percentiles; i++)
        {
            hb_bitstream_put_bits(&bs, params->distribution_maxrgb[i].percentage, 7);
            hb_bitstream_put_bits(&bs, params->distribution_maxrgb[i].percentile.num, 17);
        }

        hb_bitstream_put_bits(&bs, params->fraction_bright_pixels.num, 10);
    }

    hb_bitstream_put_bits(&bs, s->mastering_display_actual_peak_luminance_flag, 1);

    if (s->mastering_display_actual_peak_luminance_flag)
    {
        hb_bitstream_put_bits(&bs, s->num_rows_mastering_display_actual_peak_luminance, 5);
        hb_bitstream_put_bits(&bs, s->num_cols_mastering_display_actual_peak_luminance, 5);

        for (int i = 0; i < s->num_rows_mastering_display_actual_peak_luminance; i++)
        {
            for (int j = 0; j < s->num_cols_mastering_display_actual_peak_luminance; j++)
            {
                hb_bitstream_put_bits(&bs, s->mastering_display_actual_peak_luminance[i][j].num, 4);
            }
        }
    }

    for (int w = 0; w < s->num_windows; w++)
    {
        const AVHDRPlusColorTransformParams *params = &s->params[w];

        hb_bitstream_put_bits(&bs, params->tone_mapping_flag, 1);
        if (params->tone_mapping_flag)
        {
            hb_bitstream_put_bits(&bs, params->knee_point_x.num, 12);
            hb_bitstream_put_bits(&bs, params->knee_point_y.num, 12);

            hb_bitstream_put_bits(&bs, params->num_bezier_curve_anchors, 4);

            for (int i = 0; i < params->num_bezier_curve_anchors; i++)
            {
                hb_bitstream_put_bits(&bs, params->bezier_curve_anchors[i].num, 10);
            }
        }

        hb_bitstream_put_bits(&bs, params->color_saturation_mapping_flag, 1);

        if (params->color_saturation_mapping_flag)
        {
            hb_bitstream_put_bits(&bs, params->color_saturation_weight.num, 6);
        }
    }

    *buf_p = buf;
    *size = hb_bitstream_get_count_of_used_bytes(&bs);
}
