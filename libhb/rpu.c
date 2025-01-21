/* rpu.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/rpu.h"

#if HB_PROJECT_FEATURE_LIBDOVI
#include "libdovi/rpu_parser.h"
#endif

struct hb_filter_private_s
{
    int        mode;

    double     scale_factor_x;
    double     scale_factor_y;

    int        crop_top;
    int        crop_bottom;
    int        crop_left;
    int        crop_right;

    int        pad_top;
    int        pad_bottom;
    int        pad_left;
    int        pad_right;

    hb_filter_init_t         input;
    hb_filter_init_t         output;

    AVBufferRef        *rpu;
};

static int rpu_init(hb_filter_object_t *filter,
                              hb_filter_init_t   *init);


static int rpu_work(hb_filter_object_t *filter,
                              hb_buffer_t ** buf_in,
                              hb_buffer_t ** buf_out);

static void rpu_close(hb_filter_object_t *filter);

static const char rpu_template[] =
    "mode=^"HB_INT_REG"$:scale-factor-x=^"HB_FLOAT_REG"$:scale-factor-y=^"HB_FLOAT_REG"$:"
    "crop-top=^"HB_INT_REG"$:crop-bottom=^"HB_INT_REG"$:"
    "crop-left=^"HB_INT_REG"$:crop-right=^"HB_INT_REG"$:"
    "pad-top=^"HB_INT_REG"$:pad-bottom=^"HB_INT_REG"$:"
    "pad-left=^"HB_INT_REG"$:pad-right=^"HB_INT_REG"$";

hb_filter_object_t hb_filter_rpu =
{
    .id                = HB_FILTER_RPU,
    .enforce_order     = 1,
    .name              = "RPU converter",
    .settings          = NULL,
    .init              = rpu_init,
    .work              = rpu_work,
    .close             = rpu_close,
    .settings_template = rpu_template,
};


static int rpu_init(hb_filter_object_t *filter,
                    hb_filter_init_t   *init)
{
    filter->private_data = calloc(sizeof(struct hb_filter_private_s), 1);
    if (filter->private_data == NULL)
    {
        hb_error("RPU calloc failed");
        return -1;
    }
    hb_filter_private_t * pv = filter->private_data;

    pv->input = *init;

    int mode = RPU_MODE_UPDATE_ACTIVE_AREA | RPU_MODE_EMIT_UNSPECT_62_NAL;
    double scale_factor_x = 1, scale_factor_y = 1;
    int crop_top = 0, crop_bottom = 0, crop_left = 0, crop_right = 0;
    int pad_top = 0, pad_bottom = 0, pad_left = 0, pad_right = 0;

    if (filter->settings != NULL)
    {
        hb_dict_t *dict = filter->settings;
        hb_dict_extract_int(&mode, dict, "mode");

        hb_dict_extract_double(&scale_factor_x, dict, "scale-factor-x");
        hb_dict_extract_double(&scale_factor_y, dict, "scale-factor-y");

        hb_dict_extract_int(&crop_top, dict, "crop-top");
        hb_dict_extract_int(&crop_bottom, dict, "crop-bottom");
        hb_dict_extract_int(&crop_left, dict, "crop-left");
        hb_dict_extract_int(&crop_right, dict, "crop-right");

        hb_dict_extract_int(&pad_top, dict, "pad-top");
        hb_dict_extract_int(&pad_bottom, dict, "pad-bottom");
        hb_dict_extract_int(&pad_left, dict, "pad-left");
        hb_dict_extract_int(&pad_right, dict, "pad-right");
    }

    pv->mode = mode;

    pv->scale_factor_x = scale_factor_x;
    pv->scale_factor_y = scale_factor_y;

    pv->crop_top    = crop_top;
    pv->crop_bottom = crop_bottom;
    pv->crop_left   = crop_left;
    pv->crop_right  = crop_right;

    pv->pad_top    =  pad_top;
    pv->pad_bottom =  pad_bottom;
    pv->pad_left   =  pad_left;
    pv->pad_right  =  pad_right;

    pv->output = *init;

    return 0;
}

static void rpu_close(hb_filter_object_t * filter)
{
    hb_filter_private_t *pv = filter->private_data;

    if (pv == NULL)
    {
        return;
    }

    av_buffer_unref(&pv->rpu);
    free(pv);
    filter->private_data = NULL;
}

#if HB_PROJECT_FEATURE_LIBDOVI
static void apply_rpu_if_needed(hb_filter_private_t *pv, hb_buffer_t *buf)
{
    int rpu_available = 0;
    enum AVFrameSideDataType type = AV_FRAME_DATA_DOVI_RPU_BUFFER;

    for (int i = 0; i < buf->nb_side_data; i++)
    {
        const AVFrameSideData *side_data = buf->side_data[i];
        if (side_data->type == AV_FRAME_DATA_DOVI_RPU_BUFFER ||
            side_data->type == AV_FRAME_DATA_DOVI_RPU_BUFFER_T35)
        {
            type = side_data->type;
            rpu_available = 1;
        }
    }

    if (rpu_available == 0)
    {
        if (pv->rpu)
        {
            AVBufferRef *ref = av_buffer_ref(pv->rpu);
            AVFrameSideData *sd_dst = hb_buffer_new_side_data_from_buf(buf, type, ref);
            if (!sd_dst)
            {
                av_buffer_unref(&ref);
            }
            hb_log("rpu: missing rpu, falling back to last seen rpu");
        }
        else
        {
            hb_log("rpu: missing rpu, no fallback available");
        }
    }
}

static void save_rpu(hb_filter_private_t *pv, AVBufferRef *ref)
{
    av_buffer_unref(&pv->rpu);
    pv->rpu = av_buffer_ref(ref);
}
#endif

static int rpu_work(hb_filter_object_t *filter,
                    hb_buffer_t **buf_in,
                    hb_buffer_t **buf_out)
{
#if HB_PROJECT_FEATURE_LIBDOVI
    hb_filter_private_t *pv = filter->private_data;
    hb_buffer_t *in = *buf_in;

    if (in->s.flags & HB_BUF_FLAG_EOF)
    {
        *buf_out = in;
        *buf_in = NULL;
        return HB_FILTER_DONE;
    }

    // libavcodec hevc decoder seems to be missing some rpu
    // in a specific sample file, work around the issue
    // by using the last seen rpu
    apply_rpu_if_needed(pv, in);

    for (int i = 0; i < in->nb_side_data; i++)
    {
        const AVFrameSideData *side_data = in->side_data[i];
        if (side_data->type == AV_FRAME_DATA_DOVI_RPU_BUFFER ||
            side_data->type == AV_FRAME_DATA_DOVI_RPU_BUFFER_T35)
        {
            DoviRpuOpaque *rpu_in = NULL;
            if (side_data->type == AV_FRAME_DATA_DOVI_RPU_BUFFER)
            {
                rpu_in = dovi_parse_unspec62_nalu(side_data->data, side_data->size);
            }
            else if (side_data->type == AV_FRAME_DATA_DOVI_RPU_BUFFER_T35)
            {
                rpu_in = dovi_parse_itu_t35_dovi_metadata_obu(side_data->data, side_data->size);
            }

            if (rpu_in == NULL)
            {
                hb_log("rpu: dovi_parse failed");
                break;
            }

            if (pv->mode & RPU_MODE_CONVERT_TO_8_1)
            {
                const DoviRpuDataHeader *header = dovi_rpu_get_header(rpu_in);
                if (header && header->guessed_profile == 7)
                {
                    // Convert the BL to 8.1
                    int ret = dovi_convert_rpu_with_mode(rpu_in, 2);
                    if (ret < 0)
                    {
                        hb_log("rpu: dovi_convert_rpu_with_mode failed");
                    }
                }

                if (header)
                {
                    dovi_rpu_free_header(header);
                }
            }

            if (pv->mode & RPU_MODE_UPDATE_ACTIVE_AREA)
            {
                uint16_t left_offset = 0, right_offset = 0;
                uint16_t top_offset  = 0, bottom_offset = 0;

                const DoviVdrDmData *vdr_dm_data = dovi_rpu_get_vdr_dm_data(rpu_in);

                if (vdr_dm_data)
                {
                    const DoviExtMetadataBlockLevel5 *level5 = vdr_dm_data->dm_data.level5;
                    if (level5)
                    {
                        left_offset   = level5->active_area_left_offset;
                        right_offset  = level5->active_area_right_offset;
                        top_offset    = level5->active_area_top_offset;
                        bottom_offset = level5->active_area_bottom_offset;
                    }
                }

                // First subtract the crop values
                left_offset   -= left_offset   > pv->crop_left   ? pv->crop_left   : left_offset;
                right_offset  -= right_offset  > pv->crop_right  ? pv->crop_right  : right_offset;
                top_offset    -= top_offset    > pv->crop_top    ? pv->crop_right  : top_offset;
                bottom_offset -= bottom_offset > pv->crop_bottom ? pv->crop_bottom : bottom_offset;

                // Then rescale
                left_offset   = (double)left_offset   / pv->scale_factor_x;
                right_offset  = (double)right_offset  / pv->scale_factor_x;
                top_offset    = (double)top_offset    / pv->scale_factor_y;
                bottom_offset = (double)bottom_offset / pv->scale_factor_y;

                // At last add pad values
                left_offset   += pv->pad_left;
                right_offset  += pv->pad_right;
                top_offset    += pv->pad_top;
                bottom_offset += pv->pad_bottom;

                dovi_rpu_set_active_area_offsets(rpu_in,
                                                 left_offset,
                                                 right_offset,
                                                 top_offset,
                                                 bottom_offset);

                if (vdr_dm_data)
                {
                    dovi_rpu_free_vdr_dm_data(vdr_dm_data);
                }
            }

            save_rpu(pv, side_data->buf);

            if (pv->mode)
            {
                const DoviData *rpu_data = NULL;

                if (pv->mode & RPU_MODE_EMIT_UNSPECT_62_NAL)
                {
                    rpu_data = dovi_write_unspec62_nalu(rpu_in);
                }
                else if (pv->mode & RPU_MODE_EMIT_T35_OBU)
                {
                    rpu_data = dovi_write_av1_rpu_metadata_obu_t35_complete(rpu_in);
                }

                if (rpu_data)
                {
                    hb_buffer_remove_side_data(in, side_data->type);
                    const int offset = pv->mode & RPU_MODE_EMIT_UNSPECT_62_NAL ? 2 : 0;

                    AVBufferRef *ref = av_buffer_alloc(rpu_data->len - offset);
                    memcpy(ref->data, rpu_data->data + offset, rpu_data->len - offset);
                    AVFrameSideData *sd_dst = NULL;

                    if (pv->mode & RPU_MODE_EMIT_UNSPECT_62_NAL)
                    {
                        sd_dst = hb_buffer_new_side_data_from_buf(in, AV_FRAME_DATA_DOVI_RPU_BUFFER, ref);
                    }
                    else if (pv->mode & RPU_MODE_EMIT_T35_OBU)
                    {
                        sd_dst = hb_buffer_new_side_data_from_buf(in, AV_FRAME_DATA_DOVI_RPU_BUFFER_T35, ref);
                    }

                    if (!sd_dst)
                    {
                        av_buffer_unref(&ref);
                    }

                    dovi_data_free(rpu_data);
                }
                else
                {
                    hb_log("rpu: dovi_write failed");
                }
            }

            dovi_rpu_free(rpu_in);

            break;
        }
    }
#endif

    *buf_out = *buf_in;
    *buf_in = NULL;

    return HB_FILTER_OK;
}
