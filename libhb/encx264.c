/* encx264.c

   Copyright (c) 2003-2013 HandBrake Team
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

    /* If the PSNR or SSIM tunes are in use, enable the relevant metric */
    if (job->x264_tune != NULL && job->x264_tune[0] != '\0')
    {
        char *tmp = strdup(job->x264_tune);
        char *tok = strtok(tmp,   ",./-+");
        do
        {
            if (!strncasecmp(tok, "psnr", 4))
            {
                param.analyse.b_psnr = 1;
                break;
            }
            if (!strncasecmp(tok, "ssim", 4))
            {
                param.analyse.b_ssim = 1;
                break;
            }
        }
        while ((tok = strtok(NULL, ",./-+")) != NULL);
        free(tmp);
    }

    /* Some HandBrake-specific defaults; users can override them
     * using the advanced_opts string. */
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
    if (job->h264_profile != NULL && *job->h264_profile)
    {
        if (hb_apply_h264_profile(&param, job->h264_profile, 1))
        {
            free(pv);
            pv = NULL;
            return 1;
        }
    }
    if (job->h264_level != NULL && *job->h264_level)
    {
        if (hb_apply_h264_level(&param, job->h264_level,
                                job->h264_profile, 1) < 0)
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
        int uvsize = (hb_image_stride(AV_PIX_FMT_YUV420P, job->width,  1) *
                      hb_image_height(AV_PIX_FMT_YUV420P, job->height, 1));
        pv->grey_data = malloc(uvsize);
        memset(pv->grey_data, 0x80, uvsize);
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
    buf->s.duration     = get_frame_duration( pv, pic_out->i_pts );
    buf->s.start        = pic_out->i_pts;
    buf->s.stop         = buf->s.start + buf->s.duration;
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

int hb_apply_h264_profile(x264_param_t *param, const char *h264_profile,
                          int verbose)
{
    if (h264_profile != NULL &&
        strcasecmp(h264_profile, hb_h264_profile_names[0]) != 0)
    {
        /*
         * baseline profile doesn't support interlacing
         */
        if ((param->b_interlaced ||
             param->b_fake_interlaced) &&
            !strcasecmp(h264_profile, "baseline"))
        {
            if (verbose)
            {
                hb_log("hb_apply_h264_profile [warning]: baseline profile doesn't support interlacing, disabling");
            }
            param->b_interlaced = param->b_fake_interlaced = 0;
        }
        /*
         * lossless requires High 4:4:4 Predictive profile
         */
        if (param->rc.f_rf_constant < 1.0 &&
            param->rc.i_rc_method == X264_RC_CRF &&
            strcasecmp(h264_profile, "high444") != 0)
        {
            if (verbose)
            {
                hb_log("hb_apply_h264_profile [warning]: lossless requires high444 profile, disabling");
            }
            param->rc.f_rf_constant = 1.0;
        }
        if (!strcasecmp(h264_profile, "high10") ||
            !strcasecmp(h264_profile, "high422"))
        {
            // arbitrary profile names may be specified via the CLI
            // map unsupported high10 and high422 profiles to high
            return x264_param_apply_profile(param, "high");
        }
        return x264_param_apply_profile(param, h264_profile);
    }
    else if (!strcasecmp(h264_profile, hb_h264_profile_names[0]))
    {
        // "auto", do nothing
        return 0;
    }
    else
    {
        // error (profile not a string), abort
        hb_error("hb_apply_h264_profile: no profile specified");
        return -1;
    }
}

int hb_check_h264_level(const char *h264_level, int width, int height,
                        int fps_num, int fps_den, int interlaced,
                        int fake_interlaced)
{
    x264_param_t param;
    x264_param_default(&param);
    param.i_width           = width;
    param.i_height          = height;
    param.i_fps_num         = fps_num;
    param.i_fps_den         = fps_den;
    param.b_interlaced      = !!interlaced;
    param.b_fake_interlaced = !!fake_interlaced;
    return (hb_apply_h264_level(&param, h264_level, NULL, 0) != 0);
}

int hb_apply_h264_level(x264_param_t *param, const char *h264_level,
                        const char *h264_profile, int verbose)
{
    float f_framerate;
    const x264_level_t *x264_level = NULL;
    int i, i_mb_size, i_mb_rate, i_mb_width, i_mb_height, max_mb_side, ret;

    /*
     * find the x264_level_t corresponding to the requested level
     */
    if (h264_level != NULL &&
        strcasecmp(h264_level, hb_h264_level_names[0]) != 0)
    {
        for (i = 0; hb_h264_level_values[i]; i++)
        {
            if (!strcmp(hb_h264_level_names[i], h264_level))
            {
                int val = hb_h264_level_values[i];
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
    else if(!strcasecmp(h264_level, hb_h264_level_names[0]))
    {
        // "auto", do nothing
        return 0;
    }
    else
    {
        // error (level not a string), abort
        hb_error("hb_apply_h264_level: no level specified");
        return -1;
    }

    /*
     * the H.264 profile determines VBV constraints
     */
    enum
    {
        // Main or Baseline (equivalent)
        HB_ENCX264_PROFILE_MAIN,
        // High (no 4:2:2 or 10-bit support, so anything lossy is equivalent)
        HB_ENCX264_PROFILE_HIGH,
        // Lossless (4:2:0 8-bit for now)
        HB_ENCX264_PROFILE_HIGH444,
    } hb_encx264_profile;

    /*
     * H.264 profile
     *
     * TODO: we need to guess the profile like x264_sps_init does, otherwise
     * we'll get an error when setting a Main-incompatible VBV and
     * x264_sps_init() guesses Main profile. x264_sps_init() may eventually take
     * VBV into account when guessing profile, at which point this code can be
     * re-enabled.
     */
#if 0
    if (h264_profile != NULL && *h264_profile)
    {
        // if the user explicitly specified a profile, don't guess it
        if (!strcasecmp(h264_profile, "high444"))
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_HIGH444;
        }
        else if (!strcasecmp(h264_profile, "main") ||
                 !strcasecmp(h264_profile, "baseline"))
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_MAIN;
        }
        else
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_HIGH;
        }
    }
    else
#endif
    {
        // guess the H.264 profile if the user didn't request one
        if (param->rc.i_rc_method == X264_RC_CRF &&
            param->rc.f_rf_constant < 1.0)
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_HIGH444;
        }
        else if (param->analyse.b_transform_8x8 ||
                 param->i_cqm_preset != X264_CQM_FLAT)
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_HIGH;
        }
        else
        {
            hb_encx264_profile = HB_ENCX264_PROFILE_MAIN;
        }
    }

    /*
     * we need at least width and height in order to apply a level correctly
     */
    if (param->i_width <= 0 || param->i_height <= 0)
    {
        // error (invalid width or height), abort
        hb_error("hb_apply_h264_level: invalid resolution (width: %d, height: %d)",
                 param->i_width, param->i_height);
        return -1;
    }

    /*
     * a return value of 1 means there were warnings
     */
    ret = 0;

    /*
     * some levels do not support interlaced encoding
     */
    if (x264_level->frame_only && (param->b_interlaced ||
                                   param->b_fake_interlaced))
    {
        if (verbose)
        {
            hb_log("hb_apply_h264_level [warning]: interlaced flag not supported for level %s, disabling",
                   h264_level);
        }
        ret = 1;
        param->b_interlaced = param->b_fake_interlaced = 0;
    }

    /*
     * frame dimensions and rate (in macroblocks)
     */
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

    /*
     * sanitize ref/frameref
     */
    if (param->i_keyint_max != 1)
    {
        int i_max_dec_frame_buffering =
            MAX(MIN(x264_level->dpb / i_mb_size, 16), 1);
        param->i_frame_reference =
            MIN(i_max_dec_frame_buffering, param->i_frame_reference);
        /*
         * some level and resolution combos may require as little as 1 ref;
         * bframes and b-pyramid are not compatible with this scenario
         */
        if (i_max_dec_frame_buffering < 2)
        {
            param->i_bframe = 0;
        }
        else if (i_max_dec_frame_buffering < 4)
        {
            param->i_bframe_pyramid = X264_B_PYRAMID_NONE;
        }
    }

    /*
     * set and/or sanitize the VBV (if not lossless)
     */
    if (hb_encx264_profile != HB_ENCX264_PROFILE_HIGH444)
    {
        // High profile allows for higher VBV bufsize/maxrate
        int cbp_factor = hb_encx264_profile == HB_ENCX264_PROFILE_HIGH ? 5 : 4;
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

    /*
     * sanitize mvrange/mv-range
     */
    param->analyse.i_mv_range =
        MIN(param->analyse.i_mv_range,
            x264_level->mv_range >> !!param->b_interlaced);

    /*
     * TODO: check the rest of the limits
     */

    /*
     * things we can do nothing about (too late to change resolution or fps),
     * print warnings if we're not being quiet
     */
    if (x264_level->frame_size < i_mb_size)
    {
        if (verbose)
        {
            hb_log("hb_apply_h264_level [warning]: frame size (%dx%d, %d macroblocks) too high for level %s (max. %d macroblocks)",
                   i_mb_width * 16, i_mb_height * 16, i_mb_size, h264_level,
                   x264_level->frame_size);
        }
        ret = 1;
    }
    else if (x264_level->mbps < i_mb_rate)
    {
        if (verbose)
        {
            hb_log("hb_apply_h264_level [warning]: framerate (%.3f) too high for level %s at %dx%d (max. %.3f)",
                   f_framerate, h264_level, param->i_width, param->i_height,
                   (float)x264_level->mbps / i_mb_size);
        }
        ret = 1;
    }
    /*
     * width or height squared may not exceed 8 * frame_size (in macroblocks)
     * thus neither dimension may exceed sqrt(8 * frame_size)
     */
    max_mb_side = sqrt(x264_level->frame_size * 8);
    if (i_mb_width > max_mb_side)
    {
        if (verbose)
        {
            hb_log("hb_apply_h264_level [warning]: frame too wide (%d) for level %s (max. %d)",
                   param->i_width, h264_level, max_mb_side * 16);
        }
        ret = 1;
    }
    if (i_mb_height > max_mb_side)
    {
        if (verbose)
        {
            hb_log("hb_apply_h264_level [warning]: frame too tall (%d) for level %s (max. %d)",
                   param->i_height, h264_level, max_mb_side * 16);
        }
        ret = 1;
    }

    /*
     * level successfully applied, yay!
     */
    param->i_level_idc = x264_level->level_idc;
    return ret;
}

char * hb_x264_param_unparse(const char *x264_preset,  const char *x264_tune,
                             const char *x264_encopts, const char *h264_profile,
                             const char *h264_level, int width, int height)
{
    int i;
    char buf[32];
    char *unparsed_opts;
    hb_dict_t *x264_opts;
    hb_dict_entry_t *entry;
    x264_param_t defaults, param;

    /*
     * get the global x264 defaults (what we compare against)
     */
    x264_param_default(&defaults);

    /*
     * apply the defaults, preset and tune
     */
    if (x264_param_default_preset(&param, x264_preset, x264_tune) < 0)
    {
        /*
         * Note: GUIs should be able to always specifiy valid preset/tunes, so
         *       this code will hopefully never be reached
         */
        return strdup("hb_x264_param_unparse: invalid x264 preset/tune");
    }

    /*
     * place additional x264 options in a dictionary
     */
    entry     = NULL;
    x264_opts = hb_encopts_to_dict(x264_encopts, HB_VCODEC_X264);

    /*
     * some libx264 options are set via dedicated widgets in the video tab or
     * hardcoded in libhb, and have no effect when present in the advanced x264
     * options string.
     *
     * clear them from x264_opts so as to not apply then during unparse.
     */
    hb_dict_unset(&x264_opts, "qp");
    hb_dict_unset(&x264_opts, "qp_constant");
    hb_dict_unset(&x264_opts, "crf");
    hb_dict_unset(&x264_opts, "bitrate");
    hb_dict_unset(&x264_opts, "fps");
    hb_dict_unset(&x264_opts, "force-cfr");
    hb_dict_unset(&x264_opts, "sar");
    hb_dict_unset(&x264_opts, "annexb");

    /*
     * apply the additional x264 options
     */
    while ((entry = hb_dict_next(x264_opts, entry)) != NULL)
    {
        // let's not pollute GUI logs with x264_param_parse return codes
        x264_param_parse(&param, entry->key, entry->value);
    }

    /*
     * apply the x264 profile, if specified
     */
    if (h264_profile != NULL && *h264_profile)
    {
        // be quiet so at to not pollute GUI logs
        hb_apply_h264_profile(&param, h264_profile, 0);
    }

    /*
     * apply the h264 level, if specified
     */
    if (h264_level != NULL && *h264_level)
    {
        // set width/height to avoid issues in hb_apply_h264_level
        param.i_width  = width;
        param.i_height = height;
        // be quiet so at to not pollute GUI logs
        hb_apply_h264_level(&param, h264_level, h264_profile, 0);
    }

    /*
     * if x264_encopts is NULL, x264_opts wasn't initialized
     */
    if (x264_opts == NULL && (x264_opts = hb_dict_init(20)) == NULL)
    {
        return strdup("hb_x264_param_unparse: could not initialize hb_dict_t");
    }

    /*
     * x264 lets you specify some options in multiple ways. For options that we
     * do unparse, clear the forms that don't match how we unparse said option
     * from the x264_opts dictionary.
     *
     * actual synonyms are already handled by hb_encopts_to_dict().
     *
     * "no-deblock" is a special case as it can't be unparsed to "deblock=0"
     *
     * also, don't bother with forms that aren't allowed by the x264 CLI, such
     * as "no-bframes" - there are too many.
     */
    hb_dict_unset(&x264_opts, "no-sliced-threads");
    hb_dict_unset(&x264_opts, "no-scenecut");
    hb_dict_unset(&x264_opts, "no-b-adapt");
    hb_dict_unset(&x264_opts, "no-weightb");
    hb_dict_unset(&x264_opts, "no-cabac");
    hb_dict_unset(&x264_opts, "interlaced"); // we unparse to tff/bff
    hb_dict_unset(&x264_opts, "no-interlaced");
    hb_dict_unset(&x264_opts, "no-8x8dct");
    hb_dict_unset(&x264_opts, "no-mixed-refs");
    hb_dict_unset(&x264_opts, "no-fast-pskip");
    hb_dict_unset(&x264_opts, "no-dct-decimate");
    hb_dict_unset(&x264_opts, "no-psy");
    hb_dict_unset(&x264_opts, "no-mbtree");

    /*
     * compare defaults to param and unparse to the x264_opts dictionary
     */
    if (!param.b_sliced_threads != !defaults.b_sliced_threads)
    {
        // can be modified by: tune zerolatency
        sprintf(buf, "%d", !!param.b_sliced_threads);
        hb_dict_set(&x264_opts, "sliced-threads", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "sliced-threads");
    }
    if (param.i_sync_lookahead != defaults.i_sync_lookahead)
    {
        // can be modified by: tune zerolatency
        sprintf(buf, "%d", param.i_sync_lookahead);
        hb_dict_set(&x264_opts, "sync-lookahead", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "sync-lookahead");
    }
    if (param.i_level_idc != defaults.i_level_idc)
    {
        // can be modified by: level
        for (i = 0; hb_h264_level_values[i]; i++)
            if (param.i_level_idc == hb_h264_level_values[i])
                hb_dict_set(&x264_opts, "level", hb_h264_level_names[i]);
    }
    else
    {
        hb_dict_unset(&x264_opts, "level");
    }
    if (param.i_frame_reference != defaults.i_frame_reference)
    {
        // can be modified by: presets, tunes, level
        sprintf(buf, "%d", param.i_frame_reference);
        hb_dict_set(&x264_opts, "ref", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "ref");
    }
    if (param.i_scenecut_threshold != defaults.i_scenecut_threshold)
    {
        // can be modified by: preset ultrafast
        sprintf(buf, "%d", param.i_scenecut_threshold);
        hb_dict_set(&x264_opts, "scenecut", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "scenecut");
    }
    if (param.i_bframe != defaults.i_bframe)
    {
        // can be modified by: presets, tunes, profile, level
        sprintf(buf, "%d", param.i_bframe);
        hb_dict_set(&x264_opts, "bframes", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "bframes");
    }
    if (param.i_bframe > 0)
    {
        if (param.i_bframe_adaptive != defaults.i_bframe_adaptive)
        {
            // can be modified by: presets
            sprintf(buf, "%d", param.i_bframe_adaptive);
            hb_dict_set(&x264_opts, "b-adapt", buf);
        }
        else
        {
            hb_dict_unset(&x264_opts, "b-adapt");
        }
        if (param.i_bframe > 1 &&
            param.i_bframe_pyramid != defaults.i_bframe_pyramid)
        {
            // can be modified by: level
            if (param.i_bframe_pyramid < X264_B_PYRAMID_NONE)
                param.i_bframe_pyramid = X264_B_PYRAMID_NONE;
            if (param.i_bframe_pyramid > X264_B_PYRAMID_NORMAL)
                param.i_bframe_pyramid = X264_B_PYRAMID_NORMAL;
            for (i = 0; x264_b_pyramid_names[i] != NULL; i++)
                if (param.i_bframe_pyramid == i)
                    hb_dict_set(&x264_opts, "b-pyramid",
                                x264_b_pyramid_names[i]);
        }
        else
        {
            hb_dict_unset(&x264_opts, "b-pyramid");
        }
        if (param.analyse.i_direct_mv_pred != defaults.analyse.i_direct_mv_pred)
        {
            // can be modified by: presets
            if (param.analyse.i_direct_mv_pred < X264_DIRECT_PRED_NONE)
                param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_NONE;
            if (param.analyse.i_direct_mv_pred > X264_DIRECT_PRED_AUTO)
                param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
            for (i = 0; x264_direct_pred_names[i] != NULL; i++)
                if (param.analyse.i_direct_mv_pred == i)
                    hb_dict_set(&x264_opts, "direct",
                                x264_direct_pred_names[i]);
        }
        else
        {
            hb_dict_unset(&x264_opts, "direct");
        }
        if (!param.analyse.b_weighted_bipred !=
            !defaults.analyse.b_weighted_bipred)
        {
            // can be modified by: preset ultrafast, tune fastdecode
            sprintf(buf, "%d", !!param.analyse.b_weighted_bipred);
            hb_dict_set(&x264_opts, "weightb", buf);
        }
        else
        {
            hb_dict_unset(&x264_opts, "weightb");
        }
    }
    else
    {
        // no bframes, these options have no effect
        hb_dict_unset(&x264_opts, "b-adapt");
        hb_dict_unset(&x264_opts, "b-pyramid");
        hb_dict_unset(&x264_opts, "direct");
        hb_dict_unset(&x264_opts, "weightb");
        hb_dict_unset(&x264_opts, "b-bias");
        hb_dict_unset(&x264_opts, "open-gop");
    }
    if (!param.b_deblocking_filter != !defaults.b_deblocking_filter)
    {
        // can be modified by: preset ultrafast, tune fastdecode
        sprintf(buf, "%d", !param.b_deblocking_filter);
        hb_dict_set(&x264_opts, "no-deblock", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "no-deblock");
    }
    if (param.b_deblocking_filter &&
        (param.i_deblocking_filter_alphac0 != defaults.i_deblocking_filter_alphac0 ||
         param.i_deblocking_filter_beta    != defaults.i_deblocking_filter_beta))
    {
        // can be modified by: tunes
        sprintf(buf, "%d,%d", param.i_deblocking_filter_alphac0,
                param.i_deblocking_filter_beta);
        hb_dict_set(&x264_opts, "deblock", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "deblock");
    }
    if (!param.b_cabac != !defaults.b_cabac)
    {
        // can be modified by: preset ultrafast, tune fastdecode, profile
        sprintf(buf, "%d", !!param.b_cabac);
        hb_dict_set(&x264_opts, "cabac", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "cabac");
    }
    if (param.b_interlaced != defaults.b_interlaced)
    {
        if (param.b_tff)
        {
            hb_dict_set(&x264_opts, "tff", "1");
            hb_dict_unset(&x264_opts, "bff");
        }
        else
        {
            hb_dict_set(&x264_opts, "bff", "1");
            hb_dict_unset(&x264_opts, "tff");
        }
        hb_dict_unset(&x264_opts, "fake-interlaced");
    }
    else if (param.b_fake_interlaced != defaults.b_fake_interlaced)
    {
        hb_dict_set(&x264_opts, "fake-interlaced", "1");
        hb_dict_unset(&x264_opts, "tff");
        hb_dict_unset(&x264_opts, "bff");
    }
    else
    {
        hb_dict_unset(&x264_opts, "tff");
        hb_dict_unset(&x264_opts, "bff");
        hb_dict_unset(&x264_opts, "fake-interlaced");
    }
    if (param.i_cqm_preset == defaults.i_cqm_preset &&
        param.psz_cqm_file == defaults.psz_cqm_file)
    {
        // can be reset to default by: profile
        hb_dict_unset(&x264_opts, "cqm");
        hb_dict_unset(&x264_opts, "cqm4");
        hb_dict_unset(&x264_opts, "cqm8");
        hb_dict_unset(&x264_opts, "cqm4i");
        hb_dict_unset(&x264_opts, "cqm4p");
        hb_dict_unset(&x264_opts, "cqm8i");
        hb_dict_unset(&x264_opts, "cqm8p");
        hb_dict_unset(&x264_opts, "cqm4iy");
        hb_dict_unset(&x264_opts, "cqm4ic");
        hb_dict_unset(&x264_opts, "cqm4py");
        hb_dict_unset(&x264_opts, "cqm4pc");
    }
    /*
     * Note: param.analyse.intra can only be modified directly or by using
     *       x264 --preset ultrafast, but not via the "analyse" option
     */
    if (param.analyse.inter != defaults.analyse.inter)
    {
        // can be modified by: presets, tune touhou
        if (!param.analyse.inter)
        {
            hb_dict_set(&x264_opts, "analyse", "none");
        }
        else if ((param.analyse.inter & X264_ANALYSE_I4x4)      &&
                 (param.analyse.inter & X264_ANALYSE_I8x8)      &&
                 (param.analyse.inter & X264_ANALYSE_PSUB16x16) &&
                 (param.analyse.inter & X264_ANALYSE_PSUB8x8)   &&
                 (param.analyse.inter & X264_ANALYSE_BSUB16x16))
        {
            hb_dict_set(&x264_opts, "analyse", "all");
        }
        else
        {
            sprintf(buf, "%s", "");
            if (param.analyse.inter & X264_ANALYSE_I4x4)
            {
                strcat(buf, "i4x4");
            }
            if (param.analyse.inter & X264_ANALYSE_I8x8)
            {
                if (*buf)
                    strcat(buf, ",");
                strcat(buf, "i8x8");
            }
            if (param.analyse.inter & X264_ANALYSE_PSUB16x16)
            {
                if (*buf)
                    strcat(buf, ",");
                strcat(buf, "p8x8");
            }
            if (param.analyse.inter & X264_ANALYSE_PSUB8x8)
            {
                if (*buf)
                    strcat(buf, ",");
                strcat(buf, "p4x4");
            }
            if (param.analyse.inter & X264_ANALYSE_BSUB16x16)
            {
                if (*buf)
                    strcat(buf, ",");
                strcat(buf, "b8x8");
            }
            hb_dict_set(&x264_opts, "analyse", buf);
        }
    }
    else
    {
        hb_dict_unset(&x264_opts, "analyse");
    }
    if (!param.analyse.b_transform_8x8 != !defaults.analyse.b_transform_8x8)
    {
        // can be modified by: preset ultrafast, profile
        sprintf(buf, "%d", !!param.analyse.b_transform_8x8);
        hb_dict_set(&x264_opts, "8x8dct", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "8x8dct");
    }
    if (param.analyse.i_weighted_pred != defaults.analyse.i_weighted_pred)
    {
        // can be modified by: presets, tune fastdecode, profile
        sprintf(buf, "%d", param.analyse.i_weighted_pred);
        hb_dict_set(&x264_opts, "weightp", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "weightp");
    }
    if (param.analyse.i_me_method != defaults.analyse.i_me_method)
    {
        // can be modified by: presets
        if (param.analyse.i_me_method < X264_ME_DIA)
            param.analyse.i_me_method = X264_ME_DIA;
        if (param.analyse.i_me_method > X264_ME_TESA)
            param.analyse.i_me_method = X264_ME_TESA;
        for (i = 0; x264_motion_est_names[i] != NULL; i++)
            if (param.analyse.i_me_method == i)
                hb_dict_set(&x264_opts, "me", x264_motion_est_names[i]);
    }
    else
    {
        hb_dict_unset(&x264_opts, "me");
    }
    if (param.analyse.i_me_range != defaults.analyse.i_me_range)
    {
        // can be modified by: presets
        sprintf(buf, "%d", param.analyse.i_me_range);
        hb_dict_set(&x264_opts, "merange", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "merange");
    }
    if (param.analyse.i_mv_range != defaults.analyse.i_mv_range)
    {
        // can be modified by: level
        sprintf(buf, "%d", param.analyse.i_mv_range);
        hb_dict_set(&x264_opts, "mvrange", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "mvrange");
    }
    if (param.analyse.i_subpel_refine > 9 && (param.rc.i_aq_mode == 0 ||
                                              param.analyse.i_trellis < 2))
    {
        // subme 10 and higher require AQ and trellis 2
        param.analyse.i_subpel_refine = 9;
    }
    if (param.analyse.i_subpel_refine != defaults.analyse.i_subpel_refine)
    {
        // can be modified by: presets
        sprintf(buf, "%d", param.analyse.i_subpel_refine);
        hb_dict_set(&x264_opts, "subme", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "subme");
    }
    if (!param.analyse.b_mixed_references !=
        !defaults.analyse.b_mixed_references)
    {
        // can be modified by: presets
        sprintf(buf, "%d", !!param.analyse.b_mixed_references);
        hb_dict_set(&x264_opts, "mixed-refs", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "mixed-refs");
    }
    if (param.analyse.i_trellis != defaults.analyse.i_trellis)
    {
        // can be modified by: presets
        sprintf(buf, "%d", param.analyse.i_trellis);
        hb_dict_set(&x264_opts, "trellis", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "trellis");
    }
    if (!param.analyse.b_fast_pskip != !defaults.analyse.b_fast_pskip)
    {
        // can be modified by: preset placebo
        sprintf(buf, "%d", !!param.analyse.b_fast_pskip);
        hb_dict_set(&x264_opts, "fast-pskip", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "fast-pskip");
    }
    if (!param.analyse.b_dct_decimate != !defaults.analyse.b_dct_decimate)
    {
        // can be modified by: tune grain
        sprintf(buf, "%d", !!param.analyse.b_dct_decimate);
        hb_dict_set(&x264_opts, "dct-decimate", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "dct-decimate");
    }
    if (!param.analyse.b_psy != !defaults.analyse.b_psy)
    {
        // can be modified by: tunes
        sprintf(buf, "%d", !!param.analyse.b_psy);
        hb_dict_set(&x264_opts, "psy", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "psy");
    }
    if (param.analyse.b_psy &&
        (param.analyse.f_psy_rd      != defaults.analyse.f_psy_rd ||
         param.analyse.f_psy_trellis != defaults.analyse.f_psy_trellis))
    {
        // can be modified by: tunes
        sprintf(buf, "%.2f,%.2f", param.analyse.f_psy_rd,
                param.analyse.f_psy_trellis);
        hb_dict_set(&x264_opts, "psy-rd", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "psy-rd");
    }
    /*
     * Note: while deadzone is incompatible with trellis, it still has a slight
     *       effect on the output even when trellis is on, so always unparse it.
     */
    if (param.analyse.i_luma_deadzone[0] != defaults.analyse.i_luma_deadzone[0])
    {
        // can be modified by: tune grain
        sprintf(buf, "%d", param.analyse.i_luma_deadzone[0]);
        hb_dict_set(&x264_opts, "deadzone-inter", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "deadzone-inter");
    }
    if (param.analyse.i_luma_deadzone[1] != defaults.analyse.i_luma_deadzone[1])
    {
        // can be modified by: tune grain
        sprintf(buf, "%d", param.analyse.i_luma_deadzone[1]);
        hb_dict_set(&x264_opts, "deadzone-intra", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "deadzone-intra");
    }
    if (param.rc.i_vbv_buffer_size != defaults.rc.i_vbv_buffer_size)
    {
        // can be modified by: level
        sprintf(buf, "%d", param.rc.i_vbv_buffer_size);
        hb_dict_set(&x264_opts, "vbv-bufsize", buf);
        if (param.rc.i_vbv_max_bitrate != defaults.rc.i_vbv_max_bitrate)
        {
            // can be modified by: level
            sprintf(buf, "%d", param.rc.i_vbv_max_bitrate);
            hb_dict_set(&x264_opts, "vbv-maxrate", buf);
        }
        else
        {
            hb_dict_unset(&x264_opts, "vbv-maxrate");
        }
    }
    else
    {
        hb_dict_unset(&x264_opts, "vbv-bufsize");
        hb_dict_unset(&x264_opts, "vbv-maxrate");
    }
    if (param.rc.f_ip_factor != defaults.rc.f_ip_factor)
    {
        // can be modified by: tune grain
        sprintf(buf, "%.2f", param.rc.f_ip_factor);
        hb_dict_set(&x264_opts, "ipratio", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "ipratio");
    }
    if (param.i_bframe > 0 && !param.rc.b_mb_tree &&
        param.rc.f_pb_factor != defaults.rc.f_pb_factor)
    {
        // can be modified by: tune grain
        sprintf(buf, "%.2f", param.rc.f_pb_factor);
        hb_dict_set(&x264_opts, "pbratio", buf);
    }
    else
    {
        // pbratio requires bframes and is incomaptible with mbtree
        hb_dict_unset(&x264_opts, "pbratio");
    }
    if (param.rc.f_qcompress != defaults.rc.f_qcompress)
    {
        // can be modified by: tune grain
        sprintf(buf, "%.2f", param.rc.f_qcompress);
        hb_dict_set(&x264_opts, "qcomp", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "qcomp");
    }
    if (param.rc.i_aq_mode != defaults.rc.i_aq_mode)
    {
        // can be modified by: preset ultrafast, tune psnr
        sprintf(buf, "%d", param.rc.i_aq_mode);
        hb_dict_set(&x264_opts, "aq-mode", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "aq-mode");
    }
    if (param.rc.i_aq_mode > 0 &&
        param.rc.f_aq_strength != defaults.rc.f_aq_strength)
    {
        // can be modified by: tunes
        sprintf(buf, "%.2f", param.rc.f_aq_strength);
        hb_dict_set(&x264_opts, "aq-strength", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "aq-strength");
    }
    if (!param.rc.b_mb_tree != !defaults.rc.b_mb_tree)
    {
        // can be modified by: presets, tune zerolatency
        sprintf(buf, "%d", !!param.rc.b_mb_tree);
        hb_dict_set(&x264_opts, "mbtree", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "mbtree");
    }
    if (param.rc.i_lookahead != defaults.rc.i_lookahead)
    {
        // can be modified by: presets, tune zerolatency
        sprintf(buf, "%d", param.rc.i_lookahead);
        hb_dict_set(&x264_opts, "rc-lookahead", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "rc-lookahead");
    }
    if (!param.b_vfr_input != !defaults.b_vfr_input)
    {
        // can be modified by: tune zerolatency
        sprintf(buf, "%d", !param.b_vfr_input);
        hb_dict_set(&x264_opts, "force-cfr", buf);
    }
    else
    {
        hb_dict_unset(&x264_opts, "force-cfr");
    }

    /* convert the x264_opts dictionary to an encopts string */
    unparsed_opts = hb_dict_to_encopts(x264_opts);
    hb_dict_free(&x264_opts);

    /* we're done */
    return unparsed_opts;
}

const char * const * hb_x264_presets()
{
    return x264_preset_names;
}

const char * const * hb_x264_tunes()
{
    return x264_tune_names;
}

const char * const * hb_h264_profiles()
{
    return hb_h264_profile_names;
}

const char * const * hb_h264_levels()
{
    return hb_h264_level_names;
}

const char * hb_x264_encopt_name(const char *name)
{
    int i;
    for (i = 0; hb_x264_encopt_synonyms[i][0] != NULL; i++)
        if (!strcmp(name, hb_x264_encopt_synonyms[i][1]))
            return hb_x264_encopt_synonyms[i][0];
    return name;
}
