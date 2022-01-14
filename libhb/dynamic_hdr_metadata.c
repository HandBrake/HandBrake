#include <string.h>
#include "handbrake/bitstream.h"
#include "libavutil/hdr_dynamic_metadata.h"

void hb_hdr_10_sidedata_to_sei(const AVFrameSideData *side_data, uint8_t **buf_p, u_int32_t *numBytes)
{
    const uint8_t countryCode = 0xB5;
    const uint16_t terminalProviderCode = 0x003C;
    const uint16_t terminalProviderOrientedCode = 0x0001;
    const uint8_t applicationIdentifier = 4;

    AVDynamicHDRPlus *s = (AVDynamicHDRPlus *)side_data->data;
    uint8_t *buf = av_mallocz(2048);
    struct bitstream_t bs;

    bitstream_init(&bs, buf, 2048);

    bitstream_put_bits(&bs, countryCode, 8);
    bitstream_put_bits(&bs, terminalProviderCode, 16);
    bitstream_put_bits(&bs, terminalProviderOrientedCode, 16);

    bitstream_put_bits(&bs, applicationIdentifier, 8);
    bitstream_put_bits(&bs, s->application_version, 8);
    bitstream_put_bits(&bs, s->num_windows, 2);

    for (int w = 1; w < s->num_windows; w++)
    {
        AVHDRPlusColorTransformParams *params = &s->params[w];

        bitstream_put_bits(&bs, params->window_upper_left_corner_x.num, 16);
        bitstream_put_bits(&bs, params->window_upper_left_corner_x.num, 16);
        bitstream_put_bits(&bs, params->window_upper_left_corner_x.num, 16);
        bitstream_put_bits(&bs, params->window_upper_left_corner_x.num, 16);

        bitstream_put_bits(&bs, params->center_of_ellipse_x, 16);
        bitstream_put_bits(&bs, params->center_of_ellipse_y, 16);
        bitstream_put_bits(&bs, params->rotation_angle, 8);
        bitstream_put_bits(&bs, params->semimajor_axis_internal_ellipse, 16);
        bitstream_put_bits(&bs, params->semimajor_axis_external_ellipse, 16);
        bitstream_put_bits(&bs, params->semiminor_axis_external_ellipse, 16);
        bitstream_put_bits(&bs, params->overlap_process_option, 1);
    }

    bitstream_put_bits(&bs, s->targeted_system_display_maximum_luminance.num, 27);
    bitstream_put_bits(&bs, s->targeted_system_display_actual_peak_luminance_flag, 1);

    if (s->targeted_system_display_actual_peak_luminance_flag)
    {
        bitstream_put_bits(&bs, s->num_rows_targeted_system_display_actual_peak_luminance, 5);
        bitstream_put_bits(&bs, s->num_cols_targeted_system_display_actual_peak_luminance, 5);

        for (int i = 0; i < s->num_rows_targeted_system_display_actual_peak_luminance; i++)
        {
            for (int j = 0; j < s->num_cols_targeted_system_display_actual_peak_luminance; j++)
            {
                bitstream_put_bits(&bs, s->targeted_system_display_actual_peak_luminance[i][j].num, 4);
            }
        }
    }

    for (int w = 0; w < s->num_windows; w++)
    {
        AVHDRPlusColorTransformParams *params = &s->params[w];

        for (int i = 0; i < 3; i++)
        {
            bitstream_put_bits(&bs, params->maxscl[i].num, 17);
        }
        bitstream_put_bits(&bs, params->average_maxrgb.num, 17);
        bitstream_put_bits(&bs, params->num_distribution_maxrgb_percentiles, 4);

        for (int i = 0; i < params->num_distribution_maxrgb_percentiles; i++)
        {
            bitstream_put_bits(&bs, params->distribution_maxrgb[i].percentage, 7);
            bitstream_put_bits(&bs, params->distribution_maxrgb[i].percentile.num, 17);
        }

        bitstream_put_bits(&bs, params->fraction_bright_pixels.num, 10);
    }

    bitstream_put_bits(&bs, s->mastering_display_actual_peak_luminance_flag, 1);

    if (s->mastering_display_actual_peak_luminance_flag)
    {
        bitstream_put_bits(&bs, s->num_rows_mastering_display_actual_peak_luminance, 5);
        bitstream_put_bits(&bs, s->num_cols_mastering_display_actual_peak_luminance, 5);

        for (int i = 0; i < s->num_rows_mastering_display_actual_peak_luminance; i++)
        {
            for (int j = 0; j < s->num_cols_mastering_display_actual_peak_luminance; j++)
            {
                bitstream_put_bits(&bs, s->mastering_display_actual_peak_luminance[i][j].num, 4);
            }
        }
    }

    for (int w = 0; w < s->num_windows; w++)
    {
        AVHDRPlusColorTransformParams *params = &s->params[w];

        bitstream_put_bits(&bs, params->tone_mapping_flag, 1);
        if (params->tone_mapping_flag)
        {
            bitstream_put_bits(&bs, params->knee_point_x.num, 12);
            bitstream_put_bits(&bs, params->knee_point_y.num, 12);

            bitstream_put_bits(&bs, params->num_bezier_curve_anchors, 4);

            for (int i = 0; i < params->num_bezier_curve_anchors; i++)
            {
                bitstream_put_bits(&bs, params->bezier_curve_anchors[i].num, 10);
            }
        }

        bitstream_put_bits(&bs, params->color_saturation_mapping_flag, 1);

        if (params->color_saturation_mapping_flag)
        {
            bitstream_put_bits(&bs, params->color_saturation_weight.num, 6);
        }
    }

    *buf_p = buf;
    *numBytes = bitstream_get_bit_position(&bs) + (8 - bitstream_get_bit_position(&bs) % 8);
}
