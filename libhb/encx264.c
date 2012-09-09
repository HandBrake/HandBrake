/* encx264.c

   Copyright (c) 2003-2012 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdarg.h>

#include "hb.h"
#include "hb_dict.h"
#include "encx264.h"

int  encx264Init( hb_work_object_t *, hb_job_t * );
int  encx264Work( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encx264Close( hb_work_object_t * );

hb_work_object_t hb_encx264 =
{
    WORK_ENCX264,
    "H.264/AVC encoder (libx264)",
    encx264Init,
    encx264Work,
    encx264Close
};

#define DTS_BUFFER_SIZE 32

/*
 * The frame info struct remembers information about each frame across calls
 * to x264_encoder_encode. Since frames are uniquely identified by their
 * timestamp, we use some bits of the timestamp as an index. The LSB is
 * chosen so that two successive frames will have different values in the
 * bits over any plausible range of frame rates. (Starting with bit 8 allows
 * any frame rate slower than 352fps.) The MSB determines the size of the array.
 * It is chosen so that two frames can't use the same slot during the
 * encoder's max frame delay (set by the standard as 16 frames) and so
 * that, up to some minimum frame rate, frames are guaranteed to map to
 * different slots. (An MSB of 17 which is 2^(17-8+1) = 1024 slots guarantees
 * no collisions down to a rate of .7 fps).
 */
#define FRAME_INFO_MAX2 (8)     // 2^8 = 256; 90000/256 = 352 frames/sec
#define FRAME_INFO_MIN2 (17)    // 2^17 = 128K; 90000/131072 = 1.4 frames/sec
#define FRAME_INFO_SIZE (1 << (FRAME_INFO_MIN2 - FRAME_INFO_MAX2 + 1))
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

struct hb_work_private_s
{
    hb_job_t       * job;
    x264_t         * x264;
    x264_picture_t   pic_in;
    uint8_t        * grey_data;

    uint32_t       frames_in;
    uint32_t       frames_out;
    uint32_t       frames_split; // number of frames we had to split
    int            chap_mark;   // saved chap mark when we're propagating it
    int64_t        last_stop;   // Debugging - stop time of previous input frame
    int64_t        next_chap;

    struct {
        int64_t duration;
    } frame_info[FRAME_INFO_SIZE];

    char             filename[1024];
};

/***********************************************************************
 * hb_work_encx264_init
 ***********************************************************************
 *
 **********************************************************************/
int encx264Init( hb_work_object_t * w, hb_job_t * job )
{
    x264_param_t       param;
    x264_nal_t       * nal;
    int                nal_count;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    if( x264_param_default_preset( &param, job->x264_preset, job->x264_tune ) < 0 )
    {
        free( pv );
        pv = NULL;
        return 1;
    }

    /* Some HandBrake-specific defaults; users can override them
     * using the advanced_opts string. */
    
    /* Enable metrics */
    param.analyse.b_psnr = 1;
    param.analyse.b_ssim = 1;

    /* QuickTime has trouble with very low QPs (resulting in visual artifacts).
     * Known to affect QuickTime 7, QuickTime X and iTunes.
     * Testing shows that a qpmin of 3 works. */
    param.rc.i_qp_min = 3;
    
    if( job->pass == 2 && job->cfr != 1 )
    {
        hb_interjob_t * interjob = hb_interjob_get( job->h );
        param.i_fps_num = interjob->vrate;
        param.i_fps_den = interjob->vrate_base;
    }
    else
    {
        param.i_fps_num = job->vrate;
        param.i_fps_den = job->vrate_base;
    }
    if ( job->cfr == 1 )
    {
        param.i_timebase_num   = 0;
        param.i_timebase_den   = 0;
        param.b_vfr_input = 0;
    }
    else
    {
        param.i_timebase_num   = 1;
        param.i_timebase_den   = 90000;
    }

    /* Set min:max keyframe intervals to 1:10 of fps;
     * adjust +0.5 for when fps has remainder to bump
     * { 23.976, 29.976, 59.94 } to { 24, 30, 60 }. */
    param.i_keyint_min = (int)( (double)job->vrate / (double)job->vrate_base + 0.5 );
    param.i_keyint_max = 10 * param.i_keyint_min;

    param.i_log_level  = X264_LOG_INFO;

    /* set up the VUI color model & gamma to match what the COLR atom
     * set in muxmp4.c says. See libhb/muxmp4.c for notes. */
    if( job->color_matrix_code == 4 )
    {
        // Custom
        param.vui.i_colorprim = job->color_prim;
        param.vui.i_transfer  = job->color_transfer;
        param.vui.i_colmatrix = job->color_matrix;
    }
    else if( job->color_matrix_code == 3 )
    {
        // ITU BT.709 HD content
        param.vui.i_colorprim = HB_COLR_PRI_BT709;
        param.vui.i_transfer  = HB_COLR_TRA_BT709;
        param.vui.i_colmatrix = HB_COLR_MAT_BT709;
    }
    else if( job->color_matrix_code == 2 )
    {
        // ITU BT.601 DVD or SD TV content (PAL)
        param.vui.i_colorprim = HB_COLR_PRI_EBUTECH;
        param.vui.i_transfer  = HB_COLR_TRA_BT709;
        param.vui.i_colmatrix = HB_COLR_MAT_SMPTE170M;
    }
    else if( job->color_matrix_code == 1 )
    {
        // ITU BT.601 DVD or SD TV content (NTSC)
        param.vui.i_colorprim = HB_COLR_PRI_SMPTEC;
        param.vui.i_transfer  = HB_COLR_TRA_BT709;
        param.vui.i_colmatrix = HB_COLR_MAT_SMPTE170M;
    }
    else
    {
        // detected during scan
        param.vui.i_colorprim = job->title->color_prim;
        param.vui.i_transfer  = job->title->color_transfer;
        param.vui.i_colmatrix = job->title->color_matrix;
    }

    /* place job->advanced_opts in an hb_dict_t for convenience */
    hb_dict_t * x264_opts = NULL;
    if( job->advanced_opts != NULL && *job->advanced_opts != '\0' )
    {
        x264_opts = hb_encopts_to_dict( job->advanced_opts, job->vcodec );
    }
    /* iterate through x264_opts and have libx264 parse the options for us */
    int ret;
    hb_dict_entry_t * entry = NULL;
    while( ( entry = hb_dict_next( x264_opts, entry ) ) )
    {
        /* Here's where the strings are passed to libx264 for parsing. */
        ret = x264_param_parse( &param, entry->key, entry->value );
        /* Let x264 sanity check the options for us */
        if( ret == X264_PARAM_BAD_NAME )
            hb_log( "x264 options: Unknown suboption %s", entry->key );
        if( ret == X264_PARAM_BAD_VALUE )
            hb_log( "x264 options: Bad argument %s=%s", entry->key, entry->value ? entry->value : "(null)" );
    }
    hb_dict_free( &x264_opts );

    /* Reload colorimetry settings in case custom values were set
     * in the advanced_opts string */
    job->color_matrix_code = 4;
    job->color_prim = param.vui.i_colorprim;
    job->color_transfer = param.vui.i_transfer;
    job->color_matrix = param.vui.i_colmatrix;

    /* For 25 fps sources, HandBrake's explicit keyints will match the x264 defaults:
     * min-keyint 25 (same as auto), keyint 250. */
    if( param.i_keyint_min != 25 || param.i_keyint_max != 250 )
    {
        int min_auto;

        if ( param.i_fps_num / param.i_fps_den < param.i_keyint_max / 10 )
            min_auto = param.i_fps_num / param.i_fps_den;
        else
            min_auto = param.i_keyint_max / 10;

        char min[40], max[40];
        param.i_keyint_min == X264_KEYINT_MIN_AUTO ? 
            snprintf( min, 40, "auto (%d)", min_auto ) : 
            snprintf( min, 40, "%d", param.i_keyint_min );

        param.i_keyint_max == X264_KEYINT_MAX_INFINITE ? 
            snprintf( max, 40, "infinite" ) : 
            snprintf( max, 40, "%d", param.i_keyint_max );

        hb_log( "encx264: min-keyint: %s, keyint: %s", min, max );
    }

    /* Settings which can't be overriden in the advanced_opts string
     * (muxer-specific settings, resolution, ratecontrol, etc.). */

    /* Disable annexb. Inserts size into nal header instead of start code. */
    param.b_annexb = 0;

    param.i_width  = job->width;
    param.i_height = job->height;

    if( job->anamorphic.mode )
    {
        param.vui.i_sar_width  = job->anamorphic.par_width;
        param.vui.i_sar_height = job->anamorphic.par_height;

        hb_log( "encx264: encoding with stored aspect %d/%d",
                param.vui.i_sar_width, param.vui.i_sar_height );
    }

    if( job->vquality >= 0 )
    {
        /* Constant RF */
        param.rc.i_rc_method = X264_RC_CRF;
        param.rc.f_rf_constant = job->vquality;
        hb_log( "encx264: Encoding at constant RF %f", param.rc.f_rf_constant );
    }
    else
    {
        /* Average bitrate */
        param.rc.i_rc_method = X264_RC_ABR;
        param.rc.i_bitrate = job->vbitrate;
        if( job->pass > 0 && job->pass < 3 )
        {
            memset( pv->filename, 0, 1024 );
            hb_get_tempory_filename( job->h, pv->filename, "x264.log" );
        }
        switch( job->pass )
        {
            case 1:
                param.rc.b_stat_read  = 0;
                param.rc.b_stat_write = 1;
                param.rc.psz_stat_out = pv->filename;
                break;
            case 2:
                param.rc.b_stat_read  = 1;
                param.rc.b_stat_write = 0;
                param.rc.psz_stat_in  = pv->filename;
                break;
        }
    }

    /* Apply profile and level settings last, if present. */
    if (job->x264_profile != NULL)
    {
        if (x264_param_apply_profile(&param, job->x264_profile))
        {
            free(pv);
            pv = NULL;
            return 1;
        }
    }
    if (job->h264_level != NULL)
    {
        if (hb_apply_h264_level(&param, job->width, job->height,
                                job->h264_level, job->x264_profile))
        {
            free(pv);
            pv = NULL;
            return 1;
        }
    }

    /* Turbo first pass */
    if( job->pass == 1 && job->fastfirstpass == 1 )
    {
        x264_param_apply_fastfirstpass( &param );
    }

    /* B-pyramid is enabled by default. */
    job->areBframes = 2;
    
    if( !param.i_bframe )
    {
        job->areBframes = 0;
    }
    else if( !param.i_bframe_pyramid )
    {
        job->areBframes = 1;
    }
    
    hb_deep_log( 2, "encx264: opening libx264 (pass %d)", job->pass );
    pv->x264 = x264_encoder_open( &param );
    if ( pv->x264 == NULL )
    {
        hb_error("encx264: x264_encoder_open failed.");
        free( pv );
        pv = NULL;
        return 1;
    }

    x264_encoder_headers( pv->x264, &nal, &nal_count );

    /* Sequence Parameter Set */
    memcpy(w->config->h264.sps, nal[0].p_payload + 4, nal[0].i_payload - 4);
    w->config->h264.sps_length = nal[0].i_payload - 4;

    /* Picture Parameter Set */
    memcpy(w->config->h264.pps, nal[1].p_payload + 4, nal[1].i_payload - 4);
    w->config->h264.pps_length = nal[1].i_payload - 4;

    x264_picture_init( &pv->pic_in );

    pv->pic_in.img.i_csp = X264_CSP_I420;
    pv->pic_in.img.i_plane = 3;

    if( job->grayscale )
    {
        int uvsize = hb_image_stride( PIX_FMT_YUV420P, job->width, 1 ) *
                     hb_image_height( PIX_FMT_YUV420P, job->height, 1 );
        pv->grey_data = malloc( uvsize );
        memset( pv->grey_data, 0x80, uvsize );
        pv->pic_in.img.plane[1] = pv->pic_in.img.plane[2] = pv->grey_data;
    }

    return 0;
}

void encx264Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv->frames_split )
    {
        hb_log( "encx264: %u frames had to be split (%u in, %u out)",
                pv->frames_split, pv->frames_in, pv->frames_out );
    }
    free( pv->grey_data );
    x264_encoder_close( pv->x264 );
    free( pv );
    w->private_data = NULL;

    /* TODO */
}

/*
 * see comments in definition of 'frame_info' in pv struct for description
 * of what these routines are doing.
 */
static void save_frame_info( hb_work_private_t * pv, hb_buffer_t * in )
{
    int i = (in->s.start >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    pv->frame_info[i].duration = in->s.stop - in->s.start;
}

static int64_t get_frame_duration( hb_work_private_t * pv, int64_t pts )
{
    int i = (pts >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    return pv->frame_info[i].duration;
}

static hb_buffer_t *nal_encode( hb_work_object_t *w, x264_picture_t *pic_out,
                                int i_nal, x264_nal_t *nal )
{
    hb_buffer_t *buf = NULL;
    hb_work_private_t *pv = w->private_data;
    hb_job_t *job = pv->job;

    /* Should be way too large */
    buf = hb_video_buffer_init( job->width, job->height );
    buf->size = 0;
    buf->s.frametype = 0;

    // use the pts to get the original frame's duration.
    int64_t duration  = get_frame_duration( pv, pic_out->i_pts );
    buf->s.start = pic_out->i_pts;
    buf->s.stop  = pic_out->i_pts + duration;
    buf->s.renderOffset = pic_out->i_dts;
    if ( !w->config->h264.init_delay && pic_out->i_dts < 0 )
    {
        w->config->h264.init_delay = -pic_out->i_dts;
    }

    /* Encode all the NALs we were given into buf.
       NOTE: This code assumes one video frame per NAL (but there can
             be other stuff like SPS and/or PPS). If there are multiple
             frames we only get the duration of the first which will
             eventually screw up the muxer & decoder. */
    int i;
    for( i = 0; i < i_nal; i++ )
    {
        int size = nal[i].i_payload;
        memcpy(buf->data + buf->size, nal[i].p_payload, size);
        if( size < 1 )
        {
            continue;
        }

        /* H.264 in .mp4 or .mkv */
        switch( nal[i].i_type )
        {
            /* Sequence Parameter Set & Program Parameter Set go in the
             * mp4 header so skip them here
             */
            case NAL_SPS:
            case NAL_PPS:
                continue;

            case NAL_SLICE:
            case NAL_SLICE_IDR:
            case NAL_SEI:
            default:
                break;
        }

        /* Decide what type of frame we have. */
        switch( pic_out->i_type )
        {
            case X264_TYPE_IDR:
                // Handled in b_keyframe check below.
                break;

            case X264_TYPE_I:
                buf->s.frametype = HB_FRAME_I;
                break;

            case X264_TYPE_P:
                buf->s.frametype = HB_FRAME_P;
                break;

            case X264_TYPE_B:
                buf->s.frametype = HB_FRAME_B;
                break;

        /*  This is for b-pyramid, which has reference b-frames
            However, it doesn't seem to ever be used... */
            case X264_TYPE_BREF:
                buf->s.frametype = HB_FRAME_BREF;
                break;

            // If it isn't the above, what type of frame is it??
            default:
                buf->s.frametype = 0;
                break;
        }

        /* Since libx264 doesn't tell us when b-frames are
           themselves reference frames, figure it out on our own. */
        if( (buf->s.frametype == HB_FRAME_B) &&
            (nal[i].i_ref_idc != NAL_PRIORITY_DISPOSABLE) )
            buf->s.frametype = HB_FRAME_BREF;

        /* Expose disposable bit to muxer. */
        if( nal[i].i_ref_idc == NAL_PRIORITY_DISPOSABLE )
            buf->s.flags &= ~HB_FRAME_REF;
        else
            buf->s.flags |= HB_FRAME_REF;

        // PIR has no IDR frames, but x264 marks recovery points
        // as keyframes.  So fake an IDR at these points. This flag
        // is also set for real IDR frames.
        if( pic_out->b_keyframe )
        {
            buf->s.frametype = HB_FRAME_IDR;
            /* if we have a chapter marker pending and this
               frame's presentation time stamp is at or after
               the marker's time stamp, use this as the
               chapter start. */
            if( pv->next_chap != 0 && pv->next_chap <= pic_out->i_pts )
            {
                pv->next_chap = 0;
                buf->s.new_chap = pv->chap_mark;
            }
        }

        buf->size += size;
    }
    // make sure we found at least one video frame
    if ( buf->size <= 0 )
    {
        // no video - discard the buf
        hb_buffer_close( &buf );
    }
    return buf;
}

static hb_buffer_t *x264_encode( hb_work_object_t *w, hb_buffer_t *in )
{
    hb_work_private_t *pv = w->private_data;
    hb_job_t *job = pv->job;

    /* Point x264 at our current buffers Y(UV) data.  */
    pv->pic_in.img.i_stride[0] = in->plane[0].stride;
    pv->pic_in.img.i_stride[1] = in->plane[1].stride;
    pv->pic_in.img.i_stride[2] = in->plane[2].stride;
    pv->pic_in.img.plane[0] = in->plane[0].data;
    if( !job->grayscale )
    {
        pv->pic_in.img.plane[1] = in->plane[1].data;
        pv->pic_in.img.plane[2] = in->plane[2].data;
    }

    if( in->s.new_chap && job->chapter_markers )
    {
        /* chapters have to start with an IDR frame so request that this
           frame be coded as IDR. Since there may be up to 16 frames
           currently buffered in the encoder remember the timestamp so
           when this frame finally pops out of the encoder we'll mark
           its buffer as the start of a chapter. */
        pv->pic_in.i_type = X264_TYPE_IDR;
        if( pv->next_chap == 0 )
        {
            pv->next_chap = in->s.start;
            pv->chap_mark = in->s.new_chap;
        }
        /* don't let 'work_loop' put a chapter mark on the wrong buffer */
        in->s.new_chap = 0;
    }
    else
    {
        pv->pic_in.i_type = X264_TYPE_AUTO;
    }

    /* XXX this is temporary debugging code to check that the upstream
     * modules (render & sync) have generated a continuous, self-consistent
     * frame stream with the current frame's start time equal to the
     * previous frame's stop time.
     */
    if( pv->last_stop != in->s.start )
    {
        hb_log("encx264 input continuity err: last stop %"PRId64"  start %"PRId64,
                pv->last_stop, in->s.start);
    }
    pv->last_stop = in->s.stop;

    // Remember info about this frame that we need to pass across
    // the x264_encoder_encode call (since it reorders frames).
    save_frame_info( pv, in );

    /* Feed the input PTS to x264 so it can figure out proper output PTS */
    pv->pic_in.i_pts = in->s.start;

    x264_picture_t pic_out;
    int i_nal;
    x264_nal_t *nal;

    x264_encoder_encode( pv->x264, &nal, &i_nal, &pv->pic_in, &pic_out );
    if ( i_nal > 0 )
    {
        return nal_encode( w, &pic_out, i_nal, nal );
    }
    return NULL;
}

int encx264Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;

    *buf_out = NULL;

    if( in->size <= 0 )
    {
        // EOF on input. Flush any frames still in the decoder then
        // send the eof downstream to tell the muxer we're done.
        x264_picture_t pic_out;
        int i_nal;
        x264_nal_t *nal;
        hb_buffer_t *last_buf = NULL;

        while ( x264_encoder_delayed_frames( pv->x264 ) )
        {
            x264_encoder_encode( pv->x264, &nal, &i_nal, NULL, &pic_out );
            if ( i_nal == 0 )
                continue;
            if ( i_nal < 0 )
                break;

            hb_buffer_t *buf = nal_encode( w, &pic_out, i_nal, nal );
            if ( buf )
            {
                ++pv->frames_out;
                if ( last_buf == NULL )
                    *buf_out = buf;
                else
                    last_buf->next = buf;
                last_buf = buf;
            }
        }
        // Flushed everything - add the eof to the end of the chain.
        if ( last_buf == NULL )
            *buf_out = in;
        else
            last_buf->next = in;

        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    // Not EOF - encode the packet & wrap it in a NAL
    ++pv->frames_in;
    ++pv->frames_out;
    *buf_out = x264_encode( w, in );
    return HB_WORK_OK;
}

/* Applies the restrictions of the requested H.264 level to an x264_param_t.
 *
 * Returns -1 if an invalid level (or no level) is specified. GUIs should be
 * capable of always providing a valid level.
 *
 * Does not modify resolution/framerate but warns when they exceed level limits.
 *
 * Based on a x264_param_apply_level() draft and other x264 code. */
int hb_apply_h264_level(x264_param_t *param,
                        int width, int height,
                        const char *h264_level,
                        const char *x264_profile)
{
    float f_framerate;
    const x264_level_t *x264_level = NULL;
    int i, i_mb_size, i_mb_rate, i_mb_width, i_mb_height, max_mb_side;
    enum
    {
        // the H.264 profile determines VBV constraints
        HB_H264_PROFILE_MAIN,    // Main or Baseline (equivalent)
        HB_H264_PROFILE_HIGH,    // High (we only do 8-bit for now, so anything 10-bit & lossy is equivalent)
        HB_H264_PROFILE_HIGH444, // Lossless
    } h264_profile;

    /* H.264 profile */
    //
    // TODO: FIXME
    //
    // we need to guess the profile like x264_sps_init does, otherwise we'll get
    // an error when setting a Main-incompatible VBV and x264_sps_init guesses
    // Main profile. x264_sps_init may eventually take VBV into account when
    // guessing profile, at which point this code can be re-enabled.
    //
#if 0
    if (x264_profile != NULL && *x264_profile)
    {
        // if the user explicitly specified a profile, don't guess it
        if (!strcasecmp(x264_profile, "high444"))
        {
            h264_profile = HB_H264_PROFILE_HIGH444;
        }
        else if (!strcasecmp(x264_profile, "main") ||
                 !strcasecmp(x264_profile, "baseline"))
        {
            h264_profile = HB_H264_PROFILE_MAIN;
        }
        else
        {
            h264_profile = HB_H264_PROFILE_HIGH;
        }
    }
    else
#endif
    {
        /* guess the H.264 profile if the user didn't request one */
        if (param->rc.i_rc_method == X264_RC_CRF &&
            param->rc.f_rf_constant < 1.0)
        {
            h264_profile = HB_H264_PROFILE_HIGH444;
        }
        else if (param->analyse.b_transform_8x8 ||
                 param->i_cqm_preset != X264_CQM_FLAT)
        {
            h264_profile = HB_H264_PROFILE_HIGH;
        }
        else
        {
            h264_profile = HB_H264_PROFILE_MAIN;
        }
    }

    /* find the x264_level_t corresponding to the requested level */
    if (h264_level != NULL && *h264_level)
    {
        for (i = 0; h264_level_names[i]; i++)
        {
            if (!strcmp(h264_level_names[i], h264_level))
            {
                int val = h264_level_values[i];
                for (i = 0; x264_levels[i].level_idc; i++)
                {
                    if (x264_levels[i].level_idc == val)
                    {
                        x264_level = &x264_levels[i];
                        break;
                    }
                }
                break;
            }
        }
        if (x264_level == NULL)
        {
            // error (invalid or unsupported level), abort
            hb_error("hb_apply_h264_level: invalid level %s", h264_level);
            return -1;
        }
    }
    else
    {
        // error (level not a string), abort
        hb_error("hb_apply_h264_level: no level specified");
        return -1;
    }

    /* we need at least width and height in order to apply a level correctly */
    if (width <= 0 || height <= 0)
    {
        // error (invalid width or height), abort
        hb_error("hb_apply_h264_level: invalid resolution (width: %d, height: %d)",
                 width, height);
        return -1;
    }

    /* some levels do not support interlaced encoding */
    if (x264_level->frame_only && (param->b_interlaced ||
                                   param->b_fake_interlaced))
    {
        hb_log("hb_apply_h264_level [warning]: interlaced flag not supported for level %s, disabling",
                h264_level);
        param->b_interlaced = param->b_fake_interlaced = 0;
    }

    /* frame dimensions & rate (in macroblocks) */
    i_mb_width  = (param->i_width  + 15) / 16;
    i_mb_height = (param->i_height + 15) / 16;
    if (param->b_interlaced || param->b_fake_interlaced)
    {
        // interlaced: encoded height must divide cleanly by 32
        i_mb_height = (i_mb_height + 1) & ~1;
    }
    i_mb_size = i_mb_width * i_mb_height;
    if (param->i_fps_den <= 0 || param->i_fps_num <= 0)
    {
        i_mb_rate   = 0;
        f_framerate = 0.0;
    }
    else
    {
        i_mb_rate   = (int64_t)i_mb_size * param->i_fps_num / param->i_fps_den;
        f_framerate = (float)param->i_fps_num / param->i_fps_den;
    }

    /* sanitize ref/frameref */
    if (param->i_keyint_max != 1)
    {
        int i_max_dec_frame_buffering =
            MAX(MIN(x264_level->dpb / (384 * i_mb_size), 16), 1);
        param->i_frame_reference =
            MIN(i_max_dec_frame_buffering, param->i_frame_reference);
        // some level/resolution combinations may require as little as 1 reference
        // B-frames & B-pyramid are not compatible with this scenario
        if (i_max_dec_frame_buffering < 2)
        {
            param->i_bframe = 0;
        }
        else if (i_max_dec_frame_buffering < 4)
        {
            param->i_bframe_pyramid = X264_B_PYRAMID_NONE;
        }
    }

    /* set and/or sanitize the VBV (if not lossless) */
    if (h264_profile != HB_H264_PROFILE_HIGH444)
    {
        // High profile allows for higher VBV bufsize/maxrate
        int cbp_factor = h264_profile == HB_H264_PROFILE_HIGH ? 5 : 4;
        if (!param->rc.i_vbv_max_bitrate)
        {
            param->rc.i_vbv_max_bitrate = (x264_level->bitrate * cbp_factor) / 4;
        }
        else
        {
            param->rc.i_vbv_max_bitrate =
                MIN(param->rc.i_vbv_max_bitrate,
                    (x264_level->bitrate * cbp_factor) / 4);
        }
        if (!param->rc.i_vbv_buffer_size)
        {
            param->rc.i_vbv_buffer_size = (x264_level->cpb * cbp_factor) / 4;
        }
        else
        {
            param->rc.i_vbv_buffer_size =
                MIN(param->rc.i_vbv_buffer_size,
                    (x264_level->cpb * cbp_factor) / 4);
        }
    }

    /* sanitize mvrange/mv-range */
    param->analyse.i_mv_range =
        MIN(param->analyse.i_mv_range,
            x264_level->mv_range >> !!param->b_interlaced);

    /* TODO: check the rest of the limits */

    /* things we can do nothing about (too late to change resolution or fps), print warnings */
    if (x264_level->frame_size < i_mb_size)
    {
        hb_log("hb_apply_h264_level [warning]: frame size (%dx%d, %d macroblocks) too high for level %s (max. %d macroblocks)",
               i_mb_width * 16, i_mb_height * 16, i_mb_size, h264_level,
               x264_level->frame_size);
    }
    else if (x264_level->mbps < i_mb_rate)
    {
        hb_log("hb_apply_h264_level [warning]: framerate (%.3f) too high for level %s at %dx%d (max. %.3f)",
               f_framerate, h264_level, width, height,
               (float)x264_level->mbps / i_mb_size);
    }
    // width or height squared may not exceed 8 * frame_size (in macroblocks)
    // thus neither dimension may exceed sqrt(8 * frame_size)
    max_mb_side = sqrt(x264_level->frame_size * 8);
    if (i_mb_width > max_mb_side)
    {
        hb_log("hb_apply_h264_level [warning]: frame too wide (%d) for level %s (max. %d)",
               width, h264_level, max_mb_side * 16);
    }
    if (i_mb_height > max_mb_side)
    {
        hb_log("hb_apply_h264_level [warning]: frame too tall (%d) for level %s (max. %d)",
               height, h264_level, max_mb_side * 16);
    }

    /* level successfully applied, yay! */
    param->i_level_idc = x264_level->level_idc;
    return 0;
}

const char * const * hb_x264_presets()
{
    return x264_preset_names;
}

const char * const * hb_x264_tunes()
{
    return x264_tune_names;
}

const char * const * hb_x264_profiles()
{
    return x264_profile_names;
}

const char * const * hb_h264_levels()
{
    return h264_level_names;
}

const char * hb_x264_encopt_name( const char * name )
{
    int i;
    for( i = 0; x264_encopt_synonyms[i] && x264_encopt_synonyms[i+1]; i += 2 )
        if( !strcmp( name, x264_encopt_synonyms[i+1] ) )
            return x264_encopt_synonyms[i];
    return name;
}
