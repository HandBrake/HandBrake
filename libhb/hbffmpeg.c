/* hbffmpeg.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"

static int get_frame_type(int type)
{
    switch (type)
    {
        case AV_PICTURE_TYPE_B:
            return HB_FRAME_B;

        case AV_PICTURE_TYPE_S:
        case AV_PICTURE_TYPE_P:
        case AV_PICTURE_TYPE_SP:
            return HB_FRAME_P;

        case AV_PICTURE_TYPE_BI:
        case AV_PICTURE_TYPE_SI:
        case AV_PICTURE_TYPE_I:
        default:
            return HB_FRAME_I;
    }
}

static void free_side_data(AVFrameSideData **ptr_sd)
{
    AVFrameSideData *sd = *ptr_sd;

    av_buffer_unref(&sd->buf);
    av_dict_free(&sd->metadata);
    av_freep(ptr_sd);
}

static void wipe_avframe_side_data(AVFrame *frame)
{
    for (int i = 0; i < frame->nb_side_data; i++)
    {
        free_side_data(&frame->side_data[i]);
    }
    frame->nb_side_data = 0;

    av_freep(&frame->side_data);
}

static void hb_buffer_close_callback(void *opaque, uint8_t *data)
{
    hb_buffer_t *buf = opaque;
    hb_buffer_close(&buf);
}

void hb_video_buffer_to_avframe(AVFrame *frame, hb_buffer_t **buf_in)
{
    hb_buffer_t *buf = *buf_in;

    if (buf->storage_type == AVFRAME)
    {
        av_frame_ref(frame, buf->storage);
    }
    else
    {
        // Create a refcounted AVBufferRef to avoid additional copies
        AVBufferRef *buf_ref = av_buffer_create(buf->data,
                                                buf->size,
                                                hb_buffer_close_callback,
                                                buf,
                                                0);

        frame->buf[0] = buf_ref;

        for (int pp = 0; pp <= buf->f.max_plane; pp++)
        {
            frame->data[pp] = buf->plane[pp].data;
            frame->linesize[pp] = buf->plane[pp].stride;
        }

        for (int i = 0; i < buf->nb_side_data; i++)
        {
            const AVFrameSideData *sd_src = buf->side_data[i];
            AVBufferRef *ref = av_buffer_ref(sd_src->buf);
            AVFrameSideData *sd_dst = av_frame_new_side_data_from_buf(frame, sd_src->type, ref);
            if (!sd_dst)
            {
                av_buffer_unref(&ref);
                wipe_avframe_side_data(frame);
            }
        }

        frame->extended_data = frame->data;
    }

    frame->pts              = buf->s.start;
    frame->duration         = buf->s.duration;
    frame->width            = buf->f.width;
    frame->height           = buf->f.height;
    frame->format           = buf->f.fmt;

    if (buf->s.combed)
    {
        frame->flags |= AV_FRAME_FLAG_INTERLACED;
    }
    else
    {
        frame->flags &= ~AV_FRAME_FLAG_INTERLACED;
    }

    if (buf->s.flags & PIC_FLAG_TOP_FIELD_FIRST)
    {
        frame->flags |= AV_FRAME_FLAG_TOP_FIELD_FIRST;
    }
    else
    {
        frame->flags &= ~AV_FRAME_FLAG_TOP_FIELD_FIRST;
    }

    frame->format          = buf->f.fmt;
    frame->color_primaries = hb_colr_pri_hb_to_ff(buf->f.color_prim);
    frame->color_trc       = hb_colr_tra_hb_to_ff(buf->f.color_transfer);
    frame->colorspace      = hb_colr_mat_hb_to_ff(buf->f.color_matrix);
    frame->color_range     = buf->f.color_range;
    frame->chroma_location = buf->f.chroma_location;

    if (buf->storage_type == AVFRAME)
    {
        hb_buffer_close(&buf);
    }

    *buf_in = NULL;
}

void hb_avframe_set_video_buffer_flags(hb_buffer_t * buf, AVFrame *frame,
                                       AVRational time_base)
{
    if (buf == NULL || frame == NULL)
    {
        return;
    }

    buf->s.start = av_rescale_q(frame->pts, time_base, (AVRational){1, 90000});
    buf->s.duration = frame->duration;

    if (frame->flags & AV_FRAME_FLAG_TOP_FIELD_FIRST)
    {
        buf->s.flags |= PIC_FLAG_TOP_FIELD_FIRST;
    }
    if (!(frame->flags & AV_FRAME_FLAG_INTERLACED))
    {
        buf->s.flags |= PIC_FLAG_PROGRESSIVE_FRAME;
    }
    else
    {
        buf->s.combed = HB_COMB_HEAVY;
    }
    if (frame->repeat_pict == 1)
    {
        buf->s.flags |= PIC_FLAG_REPEAT_FIRST_FIELD;
    }
    if (frame->repeat_pict == 2)
    {
        buf->s.flags |= PIC_FLAG_REPEAT_FRAME;
    }
    buf->s.frametype       = get_frame_type(frame->pict_type);
    buf->f.fmt             = frame->format;
    buf->f.color_prim      = hb_colr_pri_ff_to_hb(frame->color_primaries);
    buf->f.color_transfer  = hb_colr_tra_ff_to_hb(frame->color_trc);
    buf->f.color_matrix    = hb_colr_mat_ff_to_hb(frame->colorspace);
    buf->f.color_range     = frame->color_range;
    buf->f.chroma_location = frame->chroma_location;
}

#define HB_BUFFER_WRAP_AVFRAME 1

hb_buffer_t * hb_avframe_to_video_buffer(AVFrame *frame, AVRational time_base)
{
    hb_buffer_t *buf;

#ifdef HB_BUFFER_WRAP_AVFRAME
    // Zero-copy path
    buf = hb_buffer_wrapper_init();

    if (buf == NULL)
    {
        return NULL;
    }

    AVFrame *frame_copy = av_frame_alloc();
    if (frame_copy == NULL)
    {
        hb_buffer_close(&buf);
        return NULL;
    }

    int ret;
    ret = av_frame_ref(frame_copy, frame);

    if (ret < 0)
    {
        hb_buffer_close(&buf);
        av_frame_free(&frame_copy);
        return NULL;
    }

    buf->storage_type = AVFRAME;
    buf->storage = frame_copy;

    buf->s.type = FRAME_BUF;
    buf->f.width = frame_copy->width;
    buf->f.height = frame_copy->height;
    hb_avframe_set_video_buffer_flags(buf, frame_copy, time_base);

    buf->side_data = (void **)frame_copy->side_data;
    buf->nb_side_data = frame_copy->nb_side_data;

    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(frame_copy->format);
    for (int ii = 0; ii < desc->nb_components; ii++)
    {
        int pp = desc->comp[ii].plane;
        if (pp > buf->f.max_plane)
        {
            buf->f.max_plane = pp;
        }
    }

    for (int pp = 0; pp <= buf->f.max_plane; pp++)
    {
        buf->plane[pp].data          = frame_copy->data[pp];
        buf->plane[pp].width         = hb_image_width(buf->f.fmt, buf->f.width, pp);
        buf->plane[pp].height        = hb_image_height(buf->f.fmt, buf->f.height, pp);
        buf->plane[pp].stride        = frame_copy->linesize[pp];
        buf->plane[pp].size          = buf->plane[pp].stride * buf->plane[pp].height;

        buf->size += buf->plane[pp].size;
    }

#else
    // Memcpy to a standard hb_buffer_t
    buf = hb_frame_buffer_init(frame->format, frame->width, frame->height);
    if (buf == NULL)
    {
        return NULL;
    }

    hb_avframe_set_video_buffer_flags(buf, frame, time_base);
    int pp;
    for (pp = 0; pp <= buf->f.max_plane; pp++)
    {
        int yy;
        int stride    = buf->plane[pp].stride;
        int height    = buf->plane[pp].height;
        int linesize  = frame->linesize[pp];
        int size = linesize < stride ? ABS(linesize) : stride;
        uint8_t * dst = buf->plane[pp].data;
        uint8_t * src = frame->data[pp];
        for (yy = 0; yy < height; yy++)
        {
            memcpy(dst, src, size);
            dst += stride;
            src += linesize;
        }
    }
    for (int i = 0; i < frame->nb_side_data; i++)
    {
        const AVFrameSideData *sd_src = frame->side_data[i];
        AVBufferRef *ref = av_buffer_ref(sd_src->buf);
        AVFrameSideData *sd_dst = hb_buffer_new_side_data_from_buf(buf, sd_src->type, ref);
        if (!sd_dst)
        {
            av_buffer_unref(&ref);
            hb_buffer_wipe_side_data(buf);
        }
    }
#endif

    return buf;
}

struct SwsContext*
hb_sws_get_context(int srcW, int srcH, enum AVPixelFormat srcFormat, int srcRange,
                   int dstW, int dstH, enum AVPixelFormat dstFormat, int dstRange,
                   int flags, int colorspace)
{
    struct SwsContext * ctx;

    ctx = sws_alloc_context();
    if ( ctx )
    {
        flags |= SWS_FULL_CHR_H_INT | SWS_FULL_CHR_H_INP;

        av_opt_set_int(ctx, "srcw", srcW, 0);
        av_opt_set_int(ctx, "srch", srcH, 0);
        av_opt_set_int(ctx, "src_range", srcRange, 0);
        av_opt_set_int(ctx, "src_format", srcFormat, 0);
        av_opt_set_int(ctx, "dstw", dstW, 0);
        av_opt_set_int(ctx, "dsth", dstH, 0);
        av_opt_set_int(ctx, "dst_range", dstRange, 0);
        av_opt_set_int(ctx, "dst_format", dstFormat, 0);
        av_opt_set_int(ctx, "sws_flags", flags, 0);

        sws_setColorspaceDetails( ctx,
                      sws_getCoefficients( colorspace ), // src colorspace
                      srcRange, // src range 0 = MPG, 1 = JPG
                      sws_getCoefficients( colorspace ), // dst colorspace
                      dstRange, // dst range 0 = MPG, 1 = JPG
                      0,         // brightness
                      1 << 16,   // contrast
                      1 << 16 ); // saturation

        if (sws_init_context(ctx, NULL, NULL) < 0) {
            hb_error("Cannot initialize resampling context");
            sws_freeContext(ctx);
            ctx = NULL;
        }
    }
    return ctx;
}

int hb_sws_get_colorspace(int color_matrix)
{
    int color_space = SWS_CS_DEFAULT;

    switch (color_matrix)
    {
        case HB_COLR_MAT_SMPTE170M:
            color_space = SWS_CS_ITU601;
            break;
        case HB_COLR_MAT_SMPTE240M:
            color_space = SWS_CS_SMPTE240M;
            break;
        case HB_COLR_MAT_BT709:
            color_space = SWS_CS_ITU709;
            break;
        case HB_COLR_MAT_BT2020_CL:
        case HB_COLR_MAT_BT2020_NCL:
            color_space = SWS_CS_BT2020;
            break;
        default:
            break;
    }

    return color_space;
}

int hb_colr_pri_hb_to_ff(int colr_prim)
{
    switch (colr_prim)
    {
        case HB_COLR_PRI_BT709:
            return AVCOL_PRI_BT709;
        case HB_COLR_PRI_EBUTECH:
            return AVCOL_PRI_BT470BG;
        case HB_COLR_PRI_BT470M:
            return AVCOL_PRI_BT470M;
        case HB_COLR_PRI_SMPTEC:
            return AVCOL_PRI_SMPTE170M;
        case HB_COLR_PRI_SMPTE240M:
            return AVCOL_PRI_SMPTE240M;
        case HB_COLR_PRI_BT2020:
            return AVCOL_PRI_BT2020;
        case HB_COLR_PRI_SMPTE428:
            return AVCOL_PRI_SMPTE428;
        case HB_COLR_PRI_SMPTE431:
            return AVCOL_PRI_SMPTE431;
        case HB_COLR_PRI_SMPTE432:
            return AVCOL_PRI_SMPTE432;
        case HB_COLR_PRI_JEDEC_P22:
            return AVCOL_PRI_JEDEC_P22;
        default:
        case HB_COLR_PRI_UNDEF:
            return AVCOL_PRI_UNSPECIFIED;
    }
}

int hb_colr_tra_hb_to_ff(int colr_tra)
{
    switch (colr_tra)
    {
        case HB_COLR_TRA_BT709:
            return AVCOL_TRC_BT709;
        case HB_COLR_TRA_GAMMA22:
            return AVCOL_TRC_GAMMA22;
        case HB_COLR_TRA_GAMMA28:
            return AVCOL_TRC_GAMMA28;
        case HB_COLR_TRA_SMPTE170M:
            return AVCOL_TRC_SMPTE170M;
        case HB_COLR_TRA_SMPTE240M:
            return AVCOL_TRC_SMPTE240M;
        case HB_COLR_TRA_LINEAR:
            return AVCOL_TRC_LINEAR;
        case HB_COLR_TRA_LOG:
            return AVCOL_TRC_LOG;
        case HB_COLR_TRA_LOG_SQRT:
            return AVCOL_TRC_LOG_SQRT;
        case HB_COLR_TRA_IEC61966_2_4:
            return AVCOL_TRC_IEC61966_2_4;
        case HB_COLR_TRA_BT1361_ECG:
            return AVCOL_TRC_BT1361_ECG;
        case HB_COLR_TRA_IEC61966_2_1:
            return AVCOL_TRC_IEC61966_2_1;
        case HB_COLR_TRA_BT2020_10:
            return AVCOL_TRC_BT2020_10;
        case HB_COLR_TRA_BT2020_12:
            return AVCOL_TRC_BT2020_12;
        case HB_COLR_TRA_SMPTEST2084:
            return AVCOL_TRC_SMPTE2084;
        case HB_COLR_TRA_SMPTE428:
            return AVCOL_TRC_SMPTE428;
        case HB_COLR_TRA_ARIB_STD_B67:
            return AVCOL_TRC_ARIB_STD_B67;
        default:
        case HB_COLR_TRA_UNDEF:
            return AVCOL_TRC_UNSPECIFIED;
    }
}

int hb_colr_mat_hb_to_ff(int colr_mat)
{
    switch (colr_mat)
    {
        case HB_COLR_MAT_RGB:
            return AVCOL_SPC_RGB;
        case HB_COLR_MAT_BT709:
            return AVCOL_SPC_BT709;
        case HB_COLR_MAT_FCC:
            return AVCOL_SPC_FCC;
        case HB_COLR_MAT_BT470BG:
            return AVCOL_SPC_BT470BG;
        case HB_COLR_MAT_SMPTE170M:
            return AVCOL_SPC_SMPTE170M;
        case HB_COLR_MAT_SMPTE240M:
            return AVCOL_SPC_SMPTE240M;
        case HB_COLR_MAT_YCGCO:
            return AVCOL_SPC_YCGCO;
        case HB_COLR_MAT_BT2020_NCL:
            return AVCOL_SPC_BT2020_NCL;
        case HB_COLR_MAT_BT2020_CL:
            return AVCOL_SPC_BT2020_CL;
        case HB_COLR_MAT_SMPTE2085:
            return AVCOL_SPC_SMPTE2085;
        case HB_COLR_MAT_CD_NCL:
            return AVCOL_SPC_CHROMA_DERIVED_NCL;
        case HB_COLR_MAT_CD_CL:
            return AVCOL_SPC_CHROMA_DERIVED_CL;
        case HB_COLR_MAT_ICTCP:
            return AVCOL_SPC_ICTCP;
        default:
        case HB_COLR_MAT_UNDEF:
            return AVCOL_SPC_UNSPECIFIED;
    }
}

int hb_colr_pri_ff_to_hb(int colr_prim)
{
    switch (colr_prim)
    {
        case AVCOL_PRI_BT709:
            return HB_COLR_PRI_BT709;
        case AVCOL_PRI_BT470M:
            return HB_COLR_PRI_BT470M;
        case AVCOL_PRI_BT470BG:
            return HB_COLR_PRI_EBUTECH;
        case AVCOL_PRI_SMPTE170M:
            return HB_COLR_PRI_SMPTEC;
        case AVCOL_PRI_SMPTE240M:
            return HB_COLR_PRI_SMPTE240M;
        case AVCOL_PRI_FILM:
            return HB_COLR_PRI_FILM;
        case AVCOL_PRI_BT2020:
            return HB_COLR_PRI_BT2020;
        case AVCOL_PRI_SMPTE428:
            return HB_COLR_PRI_SMPTE428;
        case AVCOL_PRI_SMPTE431:
            return HB_COLR_PRI_SMPTE431;
        case AVCOL_PRI_SMPTE432:
            return HB_COLR_PRI_SMPTE432;
        case AVCOL_PRI_JEDEC_P22:
            return HB_COLR_PRI_JEDEC_P22;
        default:
        case AVCOL_PRI_RESERVED:
        case AVCOL_PRI_RESERVED0:
        case AVCOL_PRI_UNSPECIFIED:
            return HB_COLR_PRI_UNDEF;
    }
}

int hb_colr_tra_ff_to_hb(int colr_tra)
{
    switch (colr_tra)
    {
        case AVCOL_TRC_BT709:
            return HB_COLR_TRA_BT709;
        case AVCOL_TRC_GAMMA22:
            return HB_COLR_TRA_GAMMA22;
        case AVCOL_TRC_GAMMA28:
            return HB_COLR_TRA_GAMMA28;
        case AVCOL_TRC_SMPTE170M:
            return HB_COLR_TRA_SMPTE170M;
        case AVCOL_TRC_SMPTE240M:
            return HB_COLR_TRA_SMPTE240M;
        case AVCOL_TRC_LINEAR:
            return HB_COLR_TRA_LINEAR;
        case AVCOL_TRC_LOG:
            return HB_COLR_TRA_LOG;
        case AVCOL_TRC_LOG_SQRT:
            return HB_COLR_TRA_LOG_SQRT;
        case AVCOL_TRC_IEC61966_2_4:
            return HB_COLR_TRA_IEC61966_2_4;
        case AVCOL_TRC_BT1361_ECG:
            return HB_COLR_TRA_BT1361_ECG;
        case AVCOL_TRC_IEC61966_2_1:
            return HB_COLR_TRA_IEC61966_2_1;
        case AVCOL_TRC_BT2020_10:
            return HB_COLR_TRA_BT2020_10;
        case AVCOL_TRC_BT2020_12:
            return HB_COLR_TRA_BT2020_12;
        case AVCOL_TRC_SMPTE2084:
            return HB_COLR_TRA_SMPTEST2084;
        case AVCOL_TRC_SMPTE428:
            return HB_COLR_TRA_SMPTE428;
        case AVCOL_TRC_ARIB_STD_B67:
            return HB_COLR_TRA_ARIB_STD_B67;
        default:
        case AVCOL_TRC_UNSPECIFIED:
        case AVCOL_TRC_RESERVED:
        case AVCOL_TRC_RESERVED0:
            return HB_COLR_TRA_UNDEF;
    }
}

int hb_colr_mat_ff_to_hb(int colr_mat)
{
    switch (colr_mat)
    {
        case AVCOL_SPC_RGB:
            return HB_COLR_MAT_RGB;
        case AVCOL_SPC_BT709:
            return HB_COLR_MAT_BT709;
        case AVCOL_SPC_FCC:
            return HB_COLR_MAT_FCC;
        case AVCOL_SPC_BT470BG:
            return HB_COLR_MAT_BT470BG;
        case AVCOL_SPC_SMPTE170M:
            return HB_COLR_MAT_SMPTE170M;
        case AVCOL_SPC_SMPTE240M:
            return HB_COLR_MAT_SMPTE240M;
        case AVCOL_SPC_YCGCO:
            return HB_COLR_MAT_YCGCO;
        case AVCOL_SPC_BT2020_NCL:
            return HB_COLR_MAT_BT2020_NCL;
        case AVCOL_SPC_BT2020_CL:
            return HB_COLR_MAT_BT2020_CL;
        case AVCOL_SPC_SMPTE2085:
            return HB_COLR_MAT_SMPTE2085;
        case AVCOL_SPC_CHROMA_DERIVED_NCL:
            return HB_COLR_MAT_CD_NCL;
        case AVCOL_SPC_CHROMA_DERIVED_CL:
            return HB_COLR_MAT_CD_CL;
        case AVCOL_SPC_ICTCP:
            return HB_COLR_MAT_ICTCP;
        default:
        case AVCOL_SPC_UNSPECIFIED:
        case AVCOL_SPC_RESERVED:
            return HB_COLR_MAT_UNDEF;
    }
}

static hb_rational_t hb_rational_ff_to_hb(AVRational rational)
{
    hb_rational_t hb_rational = {rational.num, rational.den};
    return hb_rational;
}

static AVRational hb_rational_hb_to_ff(hb_rational_t rational)
{
    AVRational ff_rational = {rational.num, rational.den};
    return ff_rational;
}

hb_mastering_display_metadata_t hb_mastering_ff_to_hb(AVMasteringDisplayMetadata mastering)
{
    hb_mastering_display_metadata_t hb_mastering;

    for (int i = 0; i < 3; i++)
    {
        hb_mastering.display_primaries[i][0] = hb_rational_ff_to_hb(mastering.display_primaries[i][0]);
        hb_mastering.display_primaries[i][1] = hb_rational_ff_to_hb(mastering.display_primaries[i][1]);
    }

    hb_mastering.white_point[0] = hb_rational_ff_to_hb(mastering.white_point[0]);
    hb_mastering.white_point[1] = hb_rational_ff_to_hb(mastering.white_point[1]);

    hb_mastering.min_luminance = hb_rational_ff_to_hb(mastering.min_luminance);
    hb_mastering.max_luminance = hb_rational_ff_to_hb(mastering.max_luminance);

    hb_mastering.has_primaries = mastering.has_primaries;
    hb_mastering.has_luminance = mastering.has_luminance;

    return hb_mastering;
}

AVMasteringDisplayMetadata hb_mastering_hb_to_ff(hb_mastering_display_metadata_t mastering)
{
    AVMasteringDisplayMetadata ff_mastering;

    for (int i = 0; i < 3; i++)
    {
        ff_mastering.display_primaries[i][0] = hb_rational_hb_to_ff(mastering.display_primaries[i][0]);
        ff_mastering.display_primaries[i][1] = hb_rational_hb_to_ff(mastering.display_primaries[i][1]);
    }

    ff_mastering.white_point[0] = hb_rational_hb_to_ff(mastering.white_point[0]);
    ff_mastering.white_point[1] = hb_rational_hb_to_ff(mastering.white_point[1]);

    ff_mastering.min_luminance = hb_rational_hb_to_ff(mastering.min_luminance);
    ff_mastering.max_luminance = hb_rational_hb_to_ff(mastering.max_luminance);

    ff_mastering.has_primaries = mastering.has_primaries;
    ff_mastering.has_luminance = mastering.has_luminance;

    return ff_mastering;
}

hb_ambient_viewing_environment_metadata_t hb_ambient_ff_to_hb(AVAmbientViewingEnvironment ambient)
{
    hb_ambient_viewing_environment_metadata_t hb_ambient;

    hb_ambient.ambient_illuminance = hb_rational_ff_to_hb(ambient.ambient_illuminance);
    hb_ambient.ambient_light_x = hb_rational_ff_to_hb(ambient.ambient_light_x);
    hb_ambient.ambient_light_y = hb_rational_ff_to_hb(ambient.ambient_light_y);

    return hb_ambient;
}

AVAmbientViewingEnvironment hb_ambient_hb_to_ff(hb_ambient_viewing_environment_metadata_t ambient)
{
    AVAmbientViewingEnvironment ff_ambient;

    ff_ambient.ambient_illuminance = hb_rational_hb_to_ff(ambient.ambient_illuminance);
    ff_ambient.ambient_light_x = hb_rational_hb_to_ff(ambient.ambient_light_x);
    ff_ambient.ambient_light_y = hb_rational_hb_to_ff(ambient.ambient_light_y);

    return ff_ambient;
}

AVDOVIDecoderConfigurationRecord hb_dovi_hb_to_ff(hb_dovi_conf_t dovi)
{
    AVDOVIDecoderConfigurationRecord ff_dovi;

    ff_dovi.dv_version_major = dovi.dv_version_major;
    ff_dovi.dv_version_minor = dovi.dv_version_minor;
    ff_dovi.dv_profile = dovi.dv_profile;
    ff_dovi.dv_level = dovi.dv_level;
    ff_dovi.rpu_present_flag = dovi.rpu_present_flag;
    ff_dovi.el_present_flag = dovi.el_present_flag;
    ff_dovi.bl_present_flag = dovi.bl_present_flag;
    ff_dovi.dv_bl_signal_compatibility_id = dovi.dv_bl_signal_compatibility_id;

    return ff_dovi;
}

hb_dovi_conf_t hb_dovi_ff_to_hb(AVDOVIDecoderConfigurationRecord dovi)
{
    hb_dovi_conf_t hb_dovi;

    hb_dovi.dv_version_major = dovi.dv_version_major;
    hb_dovi.dv_version_minor = dovi.dv_version_minor;
    hb_dovi.dv_profile = dovi.dv_profile;
    hb_dovi.dv_level = dovi.dv_level;
    hb_dovi.rpu_present_flag = dovi.rpu_present_flag;
    hb_dovi.el_present_flag = dovi.el_present_flag;
    hb_dovi.bl_present_flag = dovi.bl_present_flag;
    hb_dovi.dv_bl_signal_compatibility_id = dovi.dv_bl_signal_compatibility_id;

    return hb_dovi;
}

uint64_t hb_ff_mixdown_xlat(int hb_mixdown, int *downmix_mode)
{
    uint64_t ff_layout = 0;
    int mode = AV_MATRIX_ENCODING_NONE;
    switch (hb_mixdown)
    {
        // Passthru
        case HB_AMIXDOWN_NONE:
            break;

        case HB_AMIXDOWN_MONO:
        case HB_AMIXDOWN_LEFT:
        case HB_AMIXDOWN_RIGHT:
            ff_layout = AV_CH_LAYOUT_MONO;
            break;

        case HB_AMIXDOWN_DOLBY:
            ff_layout = AV_CH_LAYOUT_STEREO;
            mode = AV_MATRIX_ENCODING_DOLBY;
            break;

        case HB_AMIXDOWN_DOLBYPLII:
            ff_layout = AV_CH_LAYOUT_STEREO;
            mode = AV_MATRIX_ENCODING_DPLII;
            break;

        case HB_AMIXDOWN_STEREO:
            ff_layout = AV_CH_LAYOUT_STEREO;
            break;

        case HB_AMIXDOWN_5POINT1:
            ff_layout = AV_CH_LAYOUT_5POINT1;
            break;

        case HB_AMIXDOWN_6POINT1:
            ff_layout = AV_CH_LAYOUT_6POINT1;
            break;

        case HB_AMIXDOWN_7POINT1:
            ff_layout = AV_CH_LAYOUT_7POINT1;
            break;

        case HB_AMIXDOWN_5_2_LFE:
            ff_layout = (AV_CH_LAYOUT_5POINT1_BACK|
                         AV_CH_FRONT_LEFT_OF_CENTER|
                         AV_CH_FRONT_RIGHT_OF_CENTER);
            break;

        default:
            ff_layout = AV_CH_LAYOUT_STEREO;
            hb_log("hb_ff_mixdown_xlat: unsupported mixdown %d", hb_mixdown);
            break;
    }
    if (downmix_mode != NULL)
        *downmix_mode = mode;
    return ff_layout;
}

/*
 * Set sample format to the request format if supported by the codec.
 * The planar/packed variant of the requested format is the next best thing.
 */
void hb_ff_set_sample_fmt(AVCodecContext *context, const AVCodec *codec,
                          enum AVSampleFormat request_sample_fmt)
{
    if (context != NULL && codec != NULL &&
        codec->type == AVMEDIA_TYPE_AUDIO && codec->sample_fmts != NULL)
    {
        const enum AVSampleFormat *fmt;
        enum AVSampleFormat next_best_fmt;

        next_best_fmt = (av_sample_fmt_is_planar(request_sample_fmt)  ?
                         av_get_packed_sample_fmt(request_sample_fmt) :
                         av_get_planar_sample_fmt(request_sample_fmt));

        context->request_sample_fmt = AV_SAMPLE_FMT_NONE;

        for (fmt = codec->sample_fmts; *fmt != AV_SAMPLE_FMT_NONE; fmt++)
        {
            if (*fmt == request_sample_fmt)
            {
                context->request_sample_fmt = request_sample_fmt;
                break;
            }
            else if (*fmt == next_best_fmt)
            {
                context->request_sample_fmt = next_best_fmt;
            }
        }

        /*
         * When encoding and AVCodec.sample_fmts exists, avcodec_open2()
         * will error out if AVCodecContext.sample_fmt isn't set.
         */
        if (context->request_sample_fmt == AV_SAMPLE_FMT_NONE)
        {
            context->request_sample_fmt = codec->sample_fmts[0];
        }
        context->sample_fmt = context->request_sample_fmt;
    }
}

int hb_av_can_use_zscale(enum AVPixelFormat pix_fmt,
                         int in_width, int in_height,
                         int out_width, int out_height)
{
    if ((in_width % 2)  != 0 || (in_height % 2)  != 0 ||
        (out_width % 2) != 0 || (out_height % 2) != 0)
    {
        return 0;
    }

    static const enum AVPixelFormat pixel_fmts[] = {
        AV_PIX_FMT_YUV410P, AV_PIX_FMT_YUV411P,
        AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV422P,
        AV_PIX_FMT_YUV440P, AV_PIX_FMT_YUV444P,
        AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUVJ422P,
        AV_PIX_FMT_YUVJ440P, AV_PIX_FMT_YUVJ444P,
        AV_PIX_FMT_YUVJ411P,
        AV_PIX_FMT_YUV420P9, AV_PIX_FMT_YUV422P9, AV_PIX_FMT_YUV444P9,
        AV_PIX_FMT_YUV420P10, AV_PIX_FMT_YUV422P10, AV_PIX_FMT_YUV444P10,
        AV_PIX_FMT_YUV420P12, AV_PIX_FMT_YUV422P12, AV_PIX_FMT_YUV444P12,
        AV_PIX_FMT_YUV420P14, AV_PIX_FMT_YUV422P14, AV_PIX_FMT_YUV444P14,
        AV_PIX_FMT_YUV420P16, AV_PIX_FMT_YUV422P16, AV_PIX_FMT_YUV444P16,
        AV_PIX_FMT_YUVA420P, AV_PIX_FMT_YUVA422P, AV_PIX_FMT_YUVA444P,
        AV_PIX_FMT_YUVA420P9, AV_PIX_FMT_YUVA422P9, AV_PIX_FMT_YUVA444P9,
        AV_PIX_FMT_YUVA420P10, AV_PIX_FMT_YUVA422P10, AV_PIX_FMT_YUVA444P10,
        AV_PIX_FMT_YUVA444P12, AV_PIX_FMT_YUVA422P12,
        AV_PIX_FMT_YUVA420P16, AV_PIX_FMT_YUVA422P16, AV_PIX_FMT_YUVA444P16,
        AV_PIX_FMT_NONE
    };

    for (int i = 0; pixel_fmts[i] != AV_PIX_FMT_NONE; i++)
    {
        if (pix_fmt == pixel_fmts[i])
        {
            return 1;
        }
    }

    return 0;
}
