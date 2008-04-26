/* $Id: encx264.c,v 1.21 2005/11/04 13:09:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <stdarg.h>

#include "hb.h"

#include "x264.h"

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
 * bits over any plausible range of frame rates. (Starting with bit 9 allows
 * any frame rate slower than 175fps.) The MSB determines the size of the array.
 * It is chosen so that two frames can't use the same slot during the
 * encoder's max frame delay (set by the standard as 16 frames) and so
 * that, up to some minimum frame rate, frames are guaranteed to map to
 * different slots. (An MSB of 16 which is 2^(16-9+1) = 256 slots guarantees
 * no collisions down to a rate of 1.4 fps).
 */
#define FRAME_INFO_MAX2 (9)     // 2^9 = 512; 90000/512 = 175 frames/sec
#define FRAME_INFO_MIN2 (16)    // 2^16 = 65536; 90000/65536 = 1.4 frames/sec
#define FRAME_INFO_SIZE (1 << (FRAME_INFO_MIN2 - FRAME_INFO_MAX2 + 1))
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

struct hb_work_private_s
{
    hb_job_t       * job;
    x264_t         * x264;
    x264_picture_t   pic_in;
    uint8_t         *x264_allocated_pic;

    int            chap_mark;   // saved chap mark when we're propagating it
    int64_t        dts_next;    // DTS start time value for next output frame
    int64_t        last_stop;   // Debugging - stop time of previous input frame
    int64_t        init_delay;
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

    memset( pv->filename, 0, 1024 );
    hb_get_tempory_filename( job->h, pv->filename, "x264.log" );

    x264_param_default( &param );

    param.i_threads    = ( hb_get_cpu_count() * 3 / 2 );
    param.i_width      = job->width;
    param.i_height     = job->height;
    param.i_fps_num    = job->vrate;
    param.i_fps_den    = job->vrate_base;

    if (job->vrate_base != 1080000)
    {
        /* If the fps isn't 25, adjust the key intervals. Add 1 because
           we want 24, not 23 with a truncated remainder.               */
        param.i_keyint_min     = (job->vrate / job->vrate_base) + 1;
        param.i_keyint_max = (10 * job->vrate / job->vrate_base) + 1;
        hb_log("encx264: keyint-min: %i, keyint-max: %i", param.i_keyint_min, param.i_keyint_max);
    }

    param.i_log_level  = X264_LOG_INFO;
    if( job->h264_level )
    {
        param.b_cabac     = 0;
        param.i_level_idc = job->h264_level;
        hb_log( "encx264: encoding at level %i",
                param.i_level_idc );
    }

    /* Slightly faster with minimal quality lost */
    param.analyse.i_subpel_refine = 4;

    /*
       	This section passes the string x264opts to libx264 for parsing into
        parameter names and values.

        The string is set up like this:
        option1=value1:option2=value 2

        So, you have to iterate through based on the colons, and then put
        the left side of the equals sign in "name" and the right side into
        "value." Then you hand those strings off to x264 for interpretation.

        This is all based on the universal x264 option handling Loren
        Merritt implemented in the Mplayer/Mencoder project.
     */

    if( job->x264opts != NULL && *job->x264opts != '\0' )
    {
        char *x264opts, *x264opts_start;

        x264opts = x264opts_start = strdup(job->x264opts);

        while( x264opts_start && *x264opts )
        {
            char *name = x264opts;
            char *value;
            int ret;

            x264opts += strcspn( x264opts, ":" );
            if( *x264opts )
            {
                *x264opts = 0;
                x264opts++;
            }

            value = strchr( name, '=' );
            if( value )
            {
                *value = 0;
                value++;
            }

            /*
               When B-frames are enabled, the max frame count increments
               by 1 (regardless of the number of B-frames). If you don't
               change the duration of the video track when you mux, libmp4
               barfs.  So, check if the x264opts are using B-frames, and
               when they are, set the boolean job->areBframes as true.
             */

            if( !( strcmp( name, "bframes" ) ) )
            {
                if( atoi( value ) > 0 )
                {
                    job->areBframes = 1;
                }
            }

            /* Note b-pyramid here, so the initial delay can be doubled */
            if( !( strcmp( name, "b-pyramid" ) ) )
            {
                if( value != NULL )
                {
                    if( atoi( value ) > 0 )
                    {
                        job->areBframes = 2;
                    }
                }
                else
                {
                    job->areBframes = 2;
                }
            }

            /* Here's where the strings are passed to libx264 for parsing. */
            ret = x264_param_parse( &param, name, value );

            /* 	Let x264 sanity check the options for us*/
            if( ret == X264_PARAM_BAD_NAME )
                hb_log( "x264 options: Unknown suboption %s", name );
            if( ret == X264_PARAM_BAD_VALUE )
                hb_log( "x264 options: Bad argument %s=%s", name, value ? value : "(null)" );
        }
        free(x264opts_start);
    }


    if( job->pixel_ratio )
    {
        param.vui.i_sar_width = job->pixel_aspect_width;
        param.vui.i_sar_height = job->pixel_aspect_height;

        hb_log( "encx264: encoding with stored aspect %d/%d",
                param.vui.i_sar_width, param.vui.i_sar_height );
    }


    if( job->vquality > 0.0 && job->vquality < 1.0 )
    {
        switch( job->crf )
        {
            case 1:
                /*Constant RF*/
                param.rc.i_rc_method = X264_RC_CRF;
                param.rc.f_rf_constant = 51 - job->vquality * 51;
                hb_log( "encx264: Encoding at constant RF %f",
                        param.rc.f_rf_constant );
                break;

            case 0:
                /*Constant QP*/
                param.rc.i_rc_method = X264_RC_CQP;
                param.rc.i_qp_constant = 51 - job->vquality * 51;
                hb_log( "encx264: encoding at constant QP %d",
                        param.rc.i_qp_constant );
                break;
        }
    }
    else if( job->vquality == 0 || job->vquality >= 1.0 )
    {
        /* Use the vquality as a raw RF or QP
          instead of treating it like a percentage. */
        switch( job->crf )
        {
            case 1:
                /*Constant RF*/
                param.rc.i_rc_method = X264_RC_CRF;
                param.rc.f_rf_constant = job->vquality;
                hb_log( "encx264: Encoding at constant RF %f",
                        param.rc.f_rf_constant );
                break;

            case 0:
                /*Constant QP*/
                param.rc.i_rc_method = X264_RC_CQP;
                param.rc.i_qp_constant = job->vquality;
                hb_log( "encx264: encoding at constant QP %d",
                        param.rc.i_qp_constant );
                break;
        }        
    }
    else
    {
        /* Rate control */
        param.rc.i_rc_method = X264_RC_ABR;
        param.rc.i_bitrate = job->vbitrate;
        switch( job->pass )
        {
            case 1:
                param.rc.b_stat_write  = 1;
                param.rc.psz_stat_out = pv->filename;
                break;
            case 2:
                param.rc.b_stat_read = 1;
                param.rc.psz_stat_in = pv->filename;
                break;
        }
    }

    hb_log( "encx264: opening libx264 (pass %d)", job->pass );
    pv->x264 = x264_encoder_open( &param );

    x264_encoder_headers( pv->x264, &nal, &nal_count );

    /* Sequence Parameter Set */
    w->config->h264.sps_length = 1 + nal[1].i_payload;
    w->config->h264.sps[0] = 0x67;
    memcpy( &w->config->h264.sps[1], nal[1].p_payload, nal[1].i_payload );

    /* Picture Parameter Set */
    w->config->h264.pps_length = 1 + nal[2].i_payload;
    w->config->h264.pps[0] = 0x68;
    memcpy( &w->config->h264.pps[1], nal[2].p_payload, nal[2].i_payload );

    x264_picture_alloc( &pv->pic_in, X264_CSP_I420,
                        job->width, job->height );

    pv->x264_allocated_pic = pv->pic_in.img.plane[0];

    pv->dts_next = -1;
    pv->next_chap = 0;

    if (job->areBframes)
    {
        /* Basic initDelay value is the clockrate divided by the FPS
           -- the length of one frame in clockticks.                  */
        pv->init_delay = (float)90000 / (float)((float)job->vrate / (float)job->vrate_base);

        /* 23.976-length frames are 3753.75 ticks long. That means 25%
           will come out as 3753, 75% will be 3754. The delay has to be
           the longest possible frame duration, 3754. However, 3753.75
           gets truncated to 3753, so if that's what it is, ++ it.     */
        if (pv->init_delay == 3753)
            pv->init_delay++;

        /* For VFR, libhb sees the FPS as 29.97, but the longest frames
           will use the duration of frames running at 23.976fps instead.. */
        if (job->vfr)
        {
            pv->init_delay = 7506;
        }

        /* The delay is 1 frames for regular b-frames, 2 for b-pyramid. */
        pv->init_delay *= job->areBframes;
    }

    return 0;
}

void encx264Close( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    /*
     * Patch the x264 allocated data back in so that x264 can free it
     * we have been using our own buffers during the encode to avoid copying.
     */
    pv->pic_in.img.plane[0] = pv->x264_allocated_pic;
    x264_picture_clean( &pv->pic_in );
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
    int i = (in->start >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    pv->frame_info[i].duration = in->stop - in->start;
}

static int64_t get_frame_duration( hb_work_private_t * pv, int64_t pts )
{
    int i = (pts >> FRAME_INFO_MAX2) & FRAME_INFO_MASK;
    return pv->frame_info[i].duration;
}

int encx264Work( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t    * job = pv->job;
    hb_buffer_t * in = *buf_in, * buf;
    x264_picture_t   pic_out;
    int           i_nal;
    x264_nal_t  * nal;
    int i;

    if( in->data )
    {
        /*
         * Point x264 at our current buffers Y(UV) data.
         */
        pv->pic_in.img.plane[0] = in->data;

        if( job->grayscale )
        {
            /* XXX x264 has currently no option for grayscale encoding */
            memset( pv->pic_in.img.plane[1], 0x80, job->width * job->height / 4 );
            memset( pv->pic_in.img.plane[2], 0x80, job->width * job->height / 4 );
        }
        else
        {
            /*
             * Point x264 at our buffers (Y)UV data
             */
            pv->pic_in.img.plane[1] = in->data + job->width * job->height;
            pv->pic_in.img.plane[2] = in->data + 5 * job->width *
                job->height / 4;
        }

        if( pv->dts_next == -1 )
        {
            /* we don't have a start time yet so use the first frame's
             * start. All other frame times will be determined by the
             * sum of the prior output frame durations in *DTS* order
             * (not by the order they arrive here). This timing change is
             * essential for VFR with b-frames but a complete nop otherwise.
             */
            pv->dts_next = in->start;
        }
        if( in->new_chap && job->chapter_markers )
        {
            /* chapters have to start with an IDR frame so request that this
               frame be coded as IDR. Since there may be up to 16 frames
               currently buffered in the encoder remember the timestamp so
               when this frame finally pops out of the encoder we'll mark
               its buffer as the start of a chapter. */
            pv->pic_in.i_type = X264_TYPE_IDR;
            if( pv->next_chap == 0 )
            {
                pv->next_chap = in->start;
                pv->chap_mark = in->new_chap;
            }
            /* don't let 'work_loop' put a chapter mark on the wrong buffer */
            in->new_chap = 0;
        }
        else
        {
            pv->pic_in.i_type = X264_TYPE_AUTO;
        }
        pv->pic_in.i_qpplus1 = 0;

        /* XXX this is temporary debugging code to check that the upstream
         * modules (render & sync) have generated a continuous, self-consistent
         * frame stream with the current frame's start time equal to the
         * previous frame's stop time.
         */
        if( pv->last_stop != in->start )
        {
            hb_log("encx264 input continuity err: last stop %lld  start %lld",
                    pv->last_stop, in->start);
        }
        pv->last_stop = in->stop;

        // Remember info about this frame that we need to pass across
        // the x264_encoder_encode call (since it reorders frames).
        save_frame_info( pv, in );

        /* Feed the input DTS to x264 so it can figure out proper output PTS */
        pv->pic_in.i_pts = in->start;

        x264_encoder_encode( pv->x264, &nal, &i_nal,
                             &pv->pic_in, &pic_out );
    }
    else
    {
        x264_encoder_encode( pv->x264, &nal, &i_nal,
                             NULL, &pic_out );
        /* No more delayed B frames */
        if( i_nal == 0 )
        {
            *buf_out = NULL;
            return HB_WORK_DONE;
        }
        else
        {
        /*  Since we output at least one more frame, drop another empty
            one onto our input fifo.  We'll keep doing this automatically
            until we stop getting frames out of the encoder. */
            hb_fifo_push(w->fifo_in, hb_buffer_init(0));
        }
    }

    if( i_nal )
    {
        /* Should be way too large */
        buf        = hb_buffer_init( 3 * job->width * job->height / 2 );
        buf->size  = 0;
        buf->frametype   = 0;

        /* Get next DTS value to use */
        int64_t dts_start = pv->dts_next;

        /* compute the stop time based on the original frame's duration */
        int64_t dts_stop  = dts_start + get_frame_duration( pv, pic_out.i_pts );
        pv->dts_next = dts_stop;

        for( i = 0; i < i_nal; i++ )
        {
            int size, data;

            data = buf->alloc - buf->size;
            if( ( size = x264_nal_encode( buf->data + buf->size, &data,
                                          1, &nal[i] ) ) < 1 )
            {
                continue;
            }

            if( job->mux & HB_MUX_AVI )
            {
                if( nal[i].i_ref_idc == NAL_PRIORITY_HIGHEST )
                {
                    buf->frametype = HB_FRAME_KEY;
                }
                buf->size += size;
                continue;
            }

            /* H.264 in .mp4 */
            switch( buf->data[buf->size+4] & 0x1f )
            {
                case 0x7:
                case 0x8:
                    /* SPS, PPS */
                    break;

                default:
                    /* H.264 in mp4 (stolen from mp4creator) */
                    buf->data[buf->size+0] = ( ( size - 4 ) >> 24 ) & 0xFF;
                    buf->data[buf->size+1] = ( ( size - 4 ) >> 16 ) & 0xFF;
                    buf->data[buf->size+2] = ( ( size - 4 ) >>  8 ) & 0xFF;
                    buf->data[buf->size+3] = ( ( size - 4 ) >>  0 ) & 0xFF;
                    switch( pic_out.i_type )
                    {
                    /*  Decide what type of frame we have. */
                        case X264_TYPE_IDR:
                            buf->frametype = HB_FRAME_IDR;
                            /* if we have a chapter marker pending and this
                               frame's presentation time stamp is at or after
                               the marker's time stamp, use this as the
                               chapter start. */
                            if( pv->next_chap != 0 && pv->next_chap <= pic_out.i_pts )
                            {
                                pv->next_chap = 0;
                                buf->new_chap = pv->chap_mark;
                            }
                            break;
                        case X264_TYPE_I:
                            buf->frametype = HB_FRAME_I;
                            break;
                        case X264_TYPE_P:
                            buf->frametype = HB_FRAME_P;
                            break;
                        case X264_TYPE_B:
                            buf->frametype = HB_FRAME_B;
                            break;
                    /*  This is for b-pyramid, which has reference b-frames
                        However, it doesn't seem to ever be used... */
                        case X264_TYPE_BREF:
                            buf->frametype = HB_FRAME_BREF;
                            break;
                    /*  If it isn't the above, what type of frame is it?? */
                        default:
                            buf->frametype = 0;
                    }

                    /* Since libx264 doesn't tell us when b-frames are
                       themselves reference frames, figure it out on our own. */
                    if( (buf->frametype == HB_FRAME_B) && (nal[i].i_ref_idc != NAL_PRIORITY_DISPOSABLE) )
                        buf->frametype = HB_FRAME_BREF;

                    /* Store the output presentation time stamp
                       from x264 for use by muxmp4 in off-setting
                       b-frames with the CTTS atom. */
                    buf->renderOffset = pic_out.i_pts - dts_start + pv->init_delay;

                    buf->size += size;
            }
        }
        /* Send out the next dts values */
        buf->start = dts_start;
        buf->stop  = dts_stop;
    }

    else
        buf = NULL;

    *buf_out = buf;

    return HB_WORK_OK;
}
