/* work.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <time.h>
#include "handbrake/handbrake.h"
#include "libavformat/avformat.h"
#include "handbrake/decomb.h"
#include "handbrake/hbavfilter.h"
#include "handbrake/dovi_common.h"
#include "handbrake/hwaccel.h"

#if HB_PROJECT_FEATURE_QSV
#include "handbrake/qsv_common.h"
#endif

#ifdef __APPLE__
#include "platform/macosx/vt_common.h"
#endif

typedef struct
{
    hb_list_t * jobs;
    hb_job_t  ** current_job;
    hb_error_code * error;
    volatile int * die;

} hb_work_t;

static void work_func(void * _work);
static void do_job( hb_job_t *);
static void filter_loop( void * );

#define FIFO_UNBOUNDED 65536
#define FIFO_UNBOUNDED_WAKE 65535
#define FIFO_LARGE 32
#define FIFO_LARGE_WAKE 16
#define FIFO_SMALL 16
#define FIFO_SMALL_WAKE 15
#define FIFO_MINI 4
#define FIFO_MINI_WAKE 3

/**
 * Allocates work object and launches work thread with work_func.
 * @param jobs Handle to hb_list_t.
 * @param die Handle to user initiated exit indicator.
 * @param error Handle to error indicator.
 */
hb_thread_t * hb_work_init( hb_list_t * jobs, volatile int * die, hb_error_code * error, hb_job_t ** job )
{
    hb_work_t * work = calloc( sizeof( hb_work_t ), 1 );

    work->jobs      = jobs;
    work->current_job = job;
    work->die       = die;
    work->error     = error;

    return hb_thread_init( "work", work_func, work, HB_LOW_PRIORITY );
}

static void InitWorkState(hb_job_t * job, int pass, int pass_count)
{
    hb_state_t state;

    memset(&state, 0, sizeof(state));
    state.state       = HB_STATE_WORKING;
    state.sequence_id = job->sequence_id;
#define p state.param.working
    p.pass_id         = job->pass_id;
    p.pass            = pass;
    p.pass_count      = pass_count;
    p.progress        = 0.0;
    p.rate_cur        = 0.0;
    p.rate_avg        = 0.0;
    p.eta_seconds     = 0;
    p.hours           = -1;
    p.minutes         = -1;
    p.seconds         = -1;
#undef p

    hb_set_state( job->h, &state );
}

static void SetWorkStateInfo(hb_job_t *job)
{
    hb_state_t state;

    if (job == NULL)
    {
        return;
    }
    hb_get_state2(job->h, &state);
    state.param.working.error        = *job->done_error;
    hb_set_state( job->h, &state );
}

/**
 * Iterates through job list and calls do_job for each job.
 * @param _work Handle work object.
 */
static void work_func( void * _work )
{
    hb_work_t  * work = _work;
    hb_job_t   * job;

    time_t t = time(NULL);
    hb_log("Starting work at: %s", asctime(localtime(&t)));
    hb_log( "%d job(s) to process", hb_list_count( work->jobs ) );

    while( !*work->die && ( job = hb_list_item( work->jobs, 0 ) ) )
    {
        hb_handle_t * h = job->h;

        hb_list_rem( work->jobs, job );
        hb_list_t * passes = hb_list_init();

        // JSON jobs get special treatment.  We want to perform the title
        // scan for the JSON job automatically.  This requires that we delay
        // filling the job struct till we have performed the title scan
        // because the default values for the job come from the title.
        if (job->json != NULL)
        {
            hb_deep_log(1, "json job:\n%s", job->json);

            // Initialize state sequence_id
            InitWorkState(job, 0, 0);
            // Perform title scan for json job
            hb_json_job_scan(job->h, job->json);

            // Expand json string to full job struct
            hb_job_t *new_job = hb_json_to_job(job->h, job->json);
            if (new_job == NULL)
            {
                hb_job_close(&job);
                hb_list_close(&passes);
                *work->error = HB_ERROR_INIT;
                *work->die = 1;
                break;
            }
            new_job->h = job->h;
            new_job->sequence_id = job->sequence_id;
            hb_job_close(&job);
            job = new_job;
        }
#if HB_PROJECT_FEATURE_QSV
        if (hb_qsv_available())
        {
            hb_qsv_setup_job(job);
        }
#endif

        hb_job_setup_passes(job->h, job, passes);
        hb_job_close(&job);

        int pass_count, pass;
        pass_count = hb_list_count(passes);
        for (pass = 0; pass < pass_count && !*work->die; pass++)
        {
            job = hb_list_item(passes, pass);
            job->die = work->die;
            job->done_error = work->error;
            *(work->current_job) = job;
            InitWorkState(job, pass + 1, pass_count);
            do_job( job );
        }
        SetWorkStateInfo(job);
        *(work->current_job) = NULL;

        // Clean job passes
        for (pass = 0; pass < pass_count; pass++)
        {
            job = hb_list_item(passes, pass);
            hb_job_close(&job);
        }
        hb_list_close(&passes);

        // Force rescan of next source processed by this hb_handle_t
        // TODO: Fix this ugly hack!
        hb_force_rescan(h);
    }

    t = time(NULL);
    hb_log("Finished work at: %s", asctime(localtime(&t)));
    free( work );
}

hb_work_object_t * hb_get_work( hb_handle_t *h, int id )
{
    hb_work_object_t * w;
    for( w = hb_objects; w; w = w->next )
    {
        if( w->id == id )
        {
            hb_work_object_t *wc = malloc( sizeof(*w) );
            *wc = *w;
            wc->h = h;
            return wc;
        }
    }
    return NULL;
}

hb_work_object_t* hb_audio_decoder(hb_handle_t *h, int codec)
{
    hb_work_object_t * w = NULL;
    if (codec & HB_ACODEC_FF_MASK)
    {
        w = hb_get_work(h, WORK_DECAVCODEC);
    }
    switch (codec)
    {
        case HB_ACODEC_LPCM:
            w = hb_get_work(h, WORK_DECLPCM);
            break;
        default:
            break;
    }
    return w;
}

hb_work_object_t* hb_video_decoder(hb_handle_t *h, int vcodec, int param, void *hw_device_ctx)
{
    hb_work_object_t * w;

    w = hb_get_work(h, vcodec);
    if (w == NULL)
    {
        hb_error("Invalid video decoder: codec %d, param %d", vcodec, param);
        return NULL;
    }
    w->codec_param = param;
    w->hw_device_ctx = hw_device_ctx;

    return w;
}

hb_work_object_t* hb_video_encoder(hb_handle_t *h, int vcodec)
{
    hb_work_object_t * w = NULL;

    switch (vcodec)
    {
        case HB_VCODEC_FFMPEG_MPEG4:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_MPEG4;
            break;
        case HB_VCODEC_FFMPEG_MPEG2:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_MPEG2VIDEO;
            break;
        case HB_VCODEC_FFMPEG_VP8:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_VP8;
            break;
        case HB_VCODEC_FFMPEG_VP9:
        case HB_VCODEC_FFMPEG_VP9_10BIT:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_VP9;
            break;
        case HB_VCODEC_X264_8BIT:
        case HB_VCODEC_X264_10BIT:
            w = hb_get_work(h, WORK_ENCX264);
            break;
        case HB_VCODEC_QSV_H264:
        case HB_VCODEC_QSV_H265:
        case HB_VCODEC_QSV_H265_10BIT:
        case HB_VCODEC_QSV_AV1:
        case HB_VCODEC_QSV_AV1_10BIT:
            w = hb_get_work(h, WORK_ENCQSV);
            break;
        case HB_VCODEC_THEORA:
            w = hb_get_work(h, WORK_ENCTHEORA);
            break;
#if HB_PROJECT_FEATURE_X265
        case HB_VCODEC_X265_8BIT:
        case HB_VCODEC_X265_10BIT:
        case HB_VCODEC_X265_12BIT:
        case HB_VCODEC_X265_16BIT:
            w = hb_get_work(h, WORK_ENCX265);
            break;
#endif
#if HB_PROJECT_FEATURE_VCE
        case HB_VCODEC_FFMPEG_VCE_H264:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_H264;
            break;
        case HB_VCODEC_FFMPEG_VCE_H265:
        case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_HEVC;
            break;
        case HB_VCODEC_FFMPEG_VCE_AV1:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_AV1;
            break;
#endif
#if HB_PROJECT_FEATURE_NVENC
        case HB_VCODEC_FFMPEG_NVENC_H264:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_H264;
            break;
        case HB_VCODEC_FFMPEG_NVENC_H265:
        case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_HEVC;
            break;
        case HB_VCODEC_FFMPEG_NVENC_AV1:
        case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_AV1;
            break;
#endif
#ifdef __APPLE__
        case HB_VCODEC_VT_H264:
        case HB_VCODEC_VT_H265:
        case HB_VCODEC_VT_H265_10BIT:
            w = hb_get_work(h, WORK_ENCVT);
            break;
#endif
#if HB_PROJECT_FEATURE_MF
        case HB_VCODEC_FFMPEG_MF_H264:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_H264;
            break;
        case HB_VCODEC_FFMPEG_MF_H265:
            w = hb_get_work(h, WORK_ENCAVCODEC);
            w->codec_param = AV_CODEC_ID_HEVC;
            break;
#endif
        case HB_VCODEC_SVT_AV1:
        case HB_VCODEC_SVT_AV1_10BIT:
            w = hb_get_work(h, WORK_ENCSVTAV1);
            break;

        default:
            hb_error("Unknown video codec (0x%x)", vcodec );
    }

    return w;
}

hb_work_object_t* hb_audio_encoder(hb_handle_t *h, int codec)
{
    if (codec & HB_ACODEC_FF_MASK)
    {
        return hb_get_work(h, WORK_ENCAVCODEC_AUDIO);
    }
    switch (codec)
    {
        case HB_ACODEC_AC3:
        case HB_ACODEC_LAME:    return hb_get_work(h, WORK_ENCAVCODEC_AUDIO);
        case HB_ACODEC_VORBIS:  return hb_get_work(h, WORK_ENCVORBIS);
        case HB_ACODEC_CA_AAC:  return hb_get_work(h, WORK_ENC_CA_AAC);
        case HB_ACODEC_CA_HAAC: return hb_get_work(h, WORK_ENC_CA_HAAC);
        default:                break;
    }
    return NULL;
}

/**
 * Displays job parameters in the debug log.
 * @param job Handle work hb_job_t.
 */
void hb_display_job_info(hb_job_t *job)
{
    int i;
    hb_title_t *title = job->title;
    hb_audio_t *audio;
    hb_subtitle_t *subtitle;

    hb_log("job configuration:");
    hb_log( " * source");

    hb_log( "   + %s", title->path );

    if( job->pts_to_start || job->pts_to_stop )
    {
        int64_t stop;
        int hr_start, min_start, hr_stop, min_stop;
        float sec_start, sec_stop;

        stop = job->pts_to_start + job->pts_to_stop;

        hr_start = job->pts_to_start / (90000 * 60 * 60);
        min_start = job->pts_to_start / (90000 * 60);
        sec_start = (float)job->pts_to_start / 90000.0 - min_start * 60;
        min_start %= 60;

        hr_stop = stop / (90000 * 60 * 60);
        min_stop = stop / (90000 * 60);
        sec_stop = (float)stop / 90000.0 - min_stop * 60;
        min_stop %= 60;

        if (job->pts_to_stop)
        {
            hb_log("   + title %d, start %02d:%02d:%05.2f stop %02d:%02d:%05.2f",
                   title->index,
                   hr_start, min_start, sec_start,
                   hr_stop,  min_stop,  sec_stop);
        }
        else
        {
            hb_log("   + title %d, start %02d:%02d:%05.2f",
                   title->index,
                   hr_start, min_start, sec_start);
        }
    }
    else if( job->frame_to_start || job->frame_to_stop )
    {
        hb_log("   + title %d, frames %d to %d", title->index,
               job->frame_to_start, job->frame_to_start +
                                    job->frame_to_stop - 1);
    }
    else
    {
        hb_log( "   + title %d, chapter(s) %d to %d", title->index,
                job->chapter_start, job->chapter_end );
    }

    if( title->container_name != NULL )
        hb_log( "   + container: %s", title->container_name);

    if( title->data_rate )
    {
        hb_log( "   + data rate: %d kbps", title->data_rate / 1000 );
    }

    hb_log( " * destination");

    hb_log( "   + %s", job->file );

    hb_log("   + container: %s", hb_container_get_long_name(job->mux));
    switch (job->mux)
    {
        case HB_MUX_AV_MP4:
            if (job->optimize)
                hb_log("     + optimized for HTTP streaming (fast start)");
            if (job->ipod_atom)
                hb_log("     + compatibility atom for iPod 5G");
            break;
        default:
            break;
    }

    if (job->align_av_start)
    {
        hb_log("     + align initial A/V stream timestamps");
    }
    if (job->inline_parameter_sets)
    {
        hb_log("     + optimized for adaptive streaming (inline parameter sets)");
    }
    if( job->chapter_markers )
    {
        hb_log( "     + chapter markers" );
    }

    hb_log(" * video track");

#if HB_PROJECT_FEATURE_QSV
    if (hb_qsv_decode_is_enabled(job))
    {
        hb_log("   + decoder: %s %d-bit (%s)",
               hb_qsv_decode_get_codec_name(title->video_codec_param), hb_get_bit_depth(job->input_pix_fmt), av_get_pix_fmt_name(job->input_pix_fmt));
    } else
#endif
    if (hb_hwaccel_decode_is_enabled(job))
    {
        hb_log("   + decoder: %s %d-bit hwaccel (%s, %s)",
               title->video_codec_name, hb_get_bit_depth(job->input_pix_fmt), av_get_pix_fmt_name(job->input_pix_fmt), av_get_pix_fmt_name(job->hw_pix_fmt));
    }
    else
    {
        hb_log("   + decoder: %s %d-bit (%s)", title->video_codec_name, hb_get_bit_depth(job->input_pix_fmt), av_get_pix_fmt_name(job->input_pix_fmt));
    }

    if( title->video_bitrate )
    {
        hb_log( "     + bitrate %d kbps", title->video_bitrate / 1000 );
    }

    // Filters can modify dimensions.  So show them first.
    if( hb_list_count( job->list_filter ) )
    {
        hb_log("   + %s", hb_list_count( job->list_filter) > 1 ? "filters" : "filter" );
        for( i = 0; i < hb_list_count( job->list_filter ); i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );
            if (filter->aliased && global_verbosity_level < 2)
            {
                continue;
            }
            char * settings = hb_filter_settings_string(filter->id,
                                                        filter->settings);
            if (settings != NULL)
                hb_log("     + %s (%s)", filter->name, settings);
            else
                hb_log("     + %s (default settings)", filter->name);
            free(settings);
            if (filter->info)
            {
                hb_filter_info_t * info;

                info = filter->info(filter);
                if (info != NULL &&
                    info->human_readable_desc != NULL &&
                    info->human_readable_desc[0] != 0)
                {
                    char * line, * pos = NULL;
                    char * tmp = strdup(info->human_readable_desc);
                    for (line = strtok_r(tmp,  "\n", &pos); line != NULL;
                         line = strtok_r(NULL, "\n", &pos))
                    {
                        hb_log("       + %s", line);
                    }
                    free(tmp);
                }
                hb_filter_info_close(&info);
            }
        }
    }

    hb_log( "   + Output geometry" );
    hb_log( "     + storage dimensions: %d x %d", job->width, job->height );
    hb_log( "     + pixel aspect ratio: %d : %d", job->par.num, job->par.den );
    hb_log( "     + display dimensions: %d x %d",
            job->width * job->par.num / job->par.den, job->height );


    if( !job->indepth_scan )
    {
        /* Video encoder */
        hb_log("   + encoder: %s",
               hb_video_encoder_get_long_name(job->vcodec));

        if (job->encoder_preset && *job->encoder_preset &&
            hb_video_encoder_get_presets(job->vcodec) != NULL)
        {
            hb_log("     + preset:  %s", job->encoder_preset);
        }
        if (job->encoder_tune && *job->encoder_tune)
        {
            switch (job->vcodec)
            {
                case HB_VCODEC_X264_8BIT:
                case HB_VCODEC_X264_10BIT:
                case HB_VCODEC_X265_8BIT:
                case HB_VCODEC_X265_10BIT:
                case HB_VCODEC_X265_12BIT:
                case HB_VCODEC_X265_16BIT:
                case HB_VCODEC_SVT_AV1:
                case HB_VCODEC_SVT_AV1_10BIT:
                case HB_VCODEC_FFMPEG_VP9:
                case HB_VCODEC_FFMPEG_VP9_10BIT:
                    hb_log("     + tune:    %s", job->encoder_tune);
                default:
                    break;
            }
        }
        if (job->encoder_options != NULL && *job->encoder_options &&
            job->vcodec != HB_VCODEC_THEORA)
        {
            hb_log("     + options: %s", job->encoder_options);
        }
        if (job->encoder_profile && *job->encoder_profile)
        {
            switch (job->vcodec)
            {
                case HB_VCODEC_X264_8BIT:
                case HB_VCODEC_X264_10BIT:
                case HB_VCODEC_X265_8BIT:
                case HB_VCODEC_X265_10BIT:
                case HB_VCODEC_X265_12BIT:
                case HB_VCODEC_X265_16BIT:
                case HB_VCODEC_QSV_H264:
                case HB_VCODEC_QSV_H265:
                case HB_VCODEC_QSV_H265_10BIT:
                case HB_VCODEC_QSV_AV1:
                case HB_VCODEC_QSV_AV1_10BIT:
                case HB_VCODEC_FFMPEG_VCE_H264:
                case HB_VCODEC_FFMPEG_VCE_H265:
                case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
                case HB_VCODEC_FFMPEG_VCE_AV1:
                case HB_VCODEC_FFMPEG_NVENC_H264:
                case HB_VCODEC_FFMPEG_NVENC_H265:
                case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
                case HB_VCODEC_FFMPEG_NVENC_AV1:
                case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
                case HB_VCODEC_VT_H264:
                case HB_VCODEC_VT_H265:
                case HB_VCODEC_VT_H265_10BIT:
                case HB_VCODEC_FFMPEG_MF_H264:
                case HB_VCODEC_FFMPEG_MF_H265:
                case HB_VCODEC_SVT_AV1:
                case HB_VCODEC_SVT_AV1_10BIT:
                    hb_log("     + profile: %s", job->encoder_profile);
                default:
                    break;
            }
        }
        if (job->encoder_level && *job->encoder_level)
        {
            switch (job->vcodec)
            {
                case HB_VCODEC_X264_8BIT:
                case HB_VCODEC_X264_10BIT:
                case HB_VCODEC_X265_8BIT:
                case HB_VCODEC_X265_10BIT:
                case HB_VCODEC_X265_12BIT:
                case HB_VCODEC_QSV_H264:
                case HB_VCODEC_QSV_H265:
                case HB_VCODEC_QSV_H265_10BIT:
                case HB_VCODEC_QSV_AV1:
                case HB_VCODEC_QSV_AV1_10BIT:
                case HB_VCODEC_FFMPEG_VCE_H264:
                case HB_VCODEC_FFMPEG_VCE_H265:
                case HB_VCODEC_FFMPEG_VCE_H265_10BIT:
                case HB_VCODEC_FFMPEG_VCE_AV1:
                case HB_VCODEC_FFMPEG_NVENC_H264:
                case HB_VCODEC_FFMPEG_NVENC_H265:
                case HB_VCODEC_FFMPEG_NVENC_H265_10BIT:
                case HB_VCODEC_FFMPEG_NVENC_AV1:
                case HB_VCODEC_FFMPEG_NVENC_AV1_10BIT:
                case HB_VCODEC_VT_H264:
                case HB_VCODEC_VT_H265_10BIT:
                case HB_VCODEC_SVT_AV1:
                case HB_VCODEC_SVT_AV1_10BIT:
                // MF h.264/h.265 currently only supports auto level
                // case HB_VCODEC_FFMPEG_MF_H264:
                // case HB_VCODEC_FFMPEG_MF_H265:
                    hb_log("     + level:   %s", job->encoder_level);
                default:
                    break;
            }
        }

        if (job->vquality > HB_INVALID_VIDEO_QUALITY)
        {
            hb_log("     + quality: %.2f (%s)", job->vquality,
                   hb_video_quality_get_name(job->vcodec));
        }
        else
        {
            hb_log( "     + bitrate: %d kbps, pass: %d", job->vbitrate, job->pass_id );
            if(job->pass_id == HB_PASS_ENCODE_ANALYSIS && job->fastanalysispass == 1 &&
               ((job->vcodec & HB_VCODEC_X264_MASK) ||
                (job->vcodec & HB_VCODEC_X265_MASK)))
            {
                hb_log( "     + fast first pass" );
                if (job->vcodec & HB_VCODEC_X264_MASK)
                {
                    hb_log( "     + options: ref=1:8x8dct=0:me=dia:trellis=0" );
                    hb_log( "                analyse=i4x4 (if originally enabled, else analyse=none)" );
                    hb_log( "                subq=2 (if originally greater than 2, else subq unchanged)" );
                }
            }
        }

        hb_log("     + color profile: %d-%d-%d",
               job->color_prim, job->color_transfer, job->color_matrix);
        hb_log("     + color range: %s",
                av_color_range_name(job->color_range));
        hb_log("     + chroma location: %s",
               av_chroma_location_name(job->chroma_location));


        if (job->color_transfer == HB_COLR_TRA_SMPTEST2084)
        {
            if (job->mastering.has_primaries || job->mastering.has_luminance)
            {
                hb_log("     + mastering display metadata: r(%5.4f,%5.4f) g(%5.4f,%5.4f) b(%5.4f %5.4f) wp(%5.4f, %5.4f) min_luminance=%f, max_luminance=%f",
                       hb_q2d(job->mastering.display_primaries[0][0]),
                       hb_q2d(job->mastering.display_primaries[0][1]),
                       hb_q2d(job->mastering.display_primaries[1][0]),
                       hb_q2d(job->mastering.display_primaries[1][1]),
                       hb_q2d(job->mastering.display_primaries[2][0]),
                       hb_q2d(job->mastering.display_primaries[2][1]),
                       hb_q2d(job->mastering.white_point[0]), hb_q2d(job->mastering.white_point[1]),
                       hb_q2d(job->mastering.min_luminance), hb_q2d(job->mastering.max_luminance));
            }
            if (job->coll.max_cll && job->coll.max_fall)
            {
                hb_log("     + content light level: max_cll=%u, max_fall=%u",
                       job->coll.max_cll,
                       job->coll.max_fall);
            }
        }

        if (job->passthru_dynamic_hdr_metadata & DOVI)
        {
            hb_log("     + dolby vision configuration record: version: %d.%d, profile: %d, level: %d, rpu flag: %d, el flag: %d, bl flag: %d, compatibility id: %d",
                   job->dovi.dv_version_major,
                   job->dovi.dv_version_minor,
                   job->dovi.dv_profile,
                   job->dovi.dv_level,
                   job->dovi.rpu_present_flag,
                   job->dovi.el_present_flag,
                   job->dovi.bl_present_flag,
                   job->dovi.dv_bl_signal_compatibility_id);
        }

        if (job->passthru_dynamic_hdr_metadata & HDR_10_PLUS)
        {
            hb_log("     + hdr10+ dynamic metadata");

        }
    }

    if (job->indepth_scan)
    {
        hb_log( " * Foreign Audio Search: %s%s%s",
                job->select_subtitle_config.dest == RENDERSUB ? "Render/Burn-in" : "Passthrough",
                job->select_subtitle_config.force ? ", Forced Only" : "",
                job->select_subtitle_config.default_track ? ", Default" : "" );
    }

    for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        subtitle = hb_list_item( job->list_subtitle, i );

        if( subtitle )
        {
            if( job->indepth_scan )
            {
                hb_log( "   + subtitle, %s (track %d, id 0x%x, %s)",
                        subtitle->lang, subtitle->track, subtitle->id,
                        subtitle->format == PICTURESUB ? "Picture" : "Text");
            }
            else if (subtitle->source == IMPORTSRT)
            {
                /* For SRT, print offset and charset too */
                hb_log(" * subtitle track %d, %s (track %d, id 0x%x, Text) -> "
                       "%s%s, offset: %"PRId64", charset: %s",
                       subtitle->out_track, subtitle->lang, subtitle->track,
                       subtitle->id,
                       subtitle->config.dest == RENDERSUB ? "Render/Burn-in"
                                                          : "Passthrough",
                       subtitle->config.default_track ? ", Default" : "",
                       subtitle->config.offset, subtitle->config.src_codeset);
            }
            else if (subtitle->source == IMPORTSSA)
            {
                /* For SSA, print offset */
                hb_log(" * subtitle track %d, %s (track %d, id 0x%x, Text) -> "
                       "%s%s, offset: %"PRId64,
                       subtitle->out_track, subtitle->lang, subtitle->track,
                       subtitle->id,
                       subtitle->config.dest == RENDERSUB ? "Render/Burn-in"
                                                          : "Passthrough",
                       subtitle->config.default_track ? ", Default" : "",
                       subtitle->config.offset);
            }
            else
            {
                hb_log(" * subtitle track %d, %s (track %d, id 0x%x, %s) -> "
                       "%s%s%s",
                       subtitle->out_track, subtitle->lang, subtitle->track,
                       subtitle->id,
                       subtitle->format == PICTURESUB ? "Picture" : "Text",
                       subtitle->config.dest == RENDERSUB ? "Render/Burn-in"
                                                          : "Passthrough",
                       subtitle->config.force ? ", Forced Only" : "",
                       subtitle->config.default_track ? ", Default" : "" );
            }
            if (subtitle->config.name )
            {
                hb_log("   + name: %s", subtitle->config.name);
            }
        }
    }

    if( !job->indepth_scan )
    {
        for( i = 0; i < hb_list_count( job->list_audio ); i++ )
        {
            audio = hb_list_item( job->list_audio, i );

            hb_log( " * audio track %d", audio->config.out.track );

            if( audio->config.out.name )
                hb_log( "   + name: %s", audio->config.out.name );

            hb_log( "   + decoder: %s (track %d, id 0x%x)", audio->config.lang.description, audio->config.in.track + 1, audio->id );

            if (audio->config.in.bitrate >= 1000)
                hb_log("     + bitrate: %d kbps, samplerate: %d Hz",
                       audio->config.in.bitrate / 1000,
                       audio->config.in.samplerate);
            else
                hb_log("     + samplerate: %d Hz",
                       audio->config.in.samplerate);

            if( audio->config.out.codec & HB_ACODEC_PASS_FLAG )
            {
                hb_log("   + %s",
                       hb_audio_encoder_get_name(audio->config.out.codec));
            }
            else
            {
                hb_log("   + mixdown: %s",
                       hb_mixdown_get_name(audio->config.out.mixdown));
                if( audio->config.out.normalize_mix_level != 0 )
                {
                    hb_log( "   + normalized mixing levels" );
                }
                if( audio->config.out.gain != 0.0 )
                {
                    hb_log( "   + gain: %.fdB", audio->config.out.gain );
                }
                if (audio->config.out.dynamic_range_compression > 0.0f &&
                    hb_audio_can_apply_drc(audio->config.in.codec,
                                           audio->config.in.codec_param,
                                           audio->config.out.codec))
                {
                    hb_log( "   + dynamic range compression: %f", audio->config.out.dynamic_range_compression );
                }
                if (hb_audio_dither_is_supported(audio->config.out.codec,
                                            audio->config.in.sample_bit_depth))
                {
                    hb_log("   + dither: %s",
                           hb_audio_dither_get_description(audio->config.out.dither_method));
                }
                hb_log("   + encoder: %s",
                       hb_audio_encoder_get_long_name(audio->config.out.codec));
                if (audio->config.out.bitrate > 0)
                {
                    hb_log("     + bitrate: %d kbps, samplerate: %d Hz",
                           audio->config.out.bitrate, audio->config.out.samplerate);
                }
                else if (audio->config.out.quality != HB_INVALID_AUDIO_QUALITY)
                {
                    hb_log("     + quality: %.2f, samplerate: %d Hz",
                           audio->config.out.quality, audio->config.out.samplerate);
                }
                else if (audio->config.out.samplerate > 0)
                {
                    hb_log("     + samplerate: %d Hz",
                           audio->config.out.samplerate);
                }
                if (audio->config.out.compression_level >= 0)
                {
                    hb_log("     + compression level: %.2f",
                           audio->config.out.compression_level);
                }
            }
        }
    }
}

/* Corrects framerates when actual duration and frame count numbers are known. */
void correct_framerate( hb_interjob_t * interjob, hb_job_t * job )
{
    if (interjob->total_time <= 0 || interjob->out_frame_count <= 0 ||
        job->cfr == 1)
    {
        // Invalid or uninitialized frame statistics
        // Or CFR output
        return;
    }

    // compute actual output vrate from first pass
    int64_t num, den;
    num = interjob->out_frame_count * 90000LL;
    den = interjob->total_time;
    hb_limit_rational64(&num, &den, num, den, INT_MAX);

    job->vrate.num = num;
    job->vrate.den = den;

    den = hb_video_framerate_get_close(&job->vrate, 2.);
    if (den > 0)
    {
        int low, high, clock;
        hb_video_framerate_get_limits(&low, &high, &clock);
        job->vrate.num = clock;
        job->vrate.den = den;
    }
    if (ABS(((double)job->orig_vrate.num /  job->orig_vrate.den) -
            ((double)     job->vrate.num /       job->vrate.den)) > 0.05)
    {
        hb_log("work: correcting framerate, %d/%d -> %d/%d",
               job->orig_vrate.num, job->orig_vrate.den,
               job->vrate.num, job->vrate.den);
    }
}

static void analyze_subtitle_scan( hb_job_t * job )
{
    hb_subtitle_t *subtitle;
    int subtitle_highest     = 0;
    int subtitle_lowest      = 0;
    int subtitle_lowest_id   = 0;
    int subtitle_forced_id   = 0;
    int subtitle_forced_hits = 0;
    int subtitle_hit         = 0;
    int i;

    // Before closing the title print out our subtitle stats if we need to
    // find the highest and lowest.
    for (i = 0; i < hb_list_count(job->list_subtitle); i++)
    {
        subtitle = hb_list_item(job->list_subtitle, i);

        hb_log("Subtitle track %d (id 0x%x) '%s': %d hits (%d forced)",
               subtitle->track, subtitle->id, subtitle->lang,
               subtitle->hits, subtitle->forced_hits);

        if (subtitle->hits == 0)
            continue;

        if (subtitle_highest < subtitle->hits)
        {
            subtitle_highest = subtitle->hits;
        }

        if (subtitle_lowest == 0 ||
            subtitle_lowest > subtitle->hits)
        {
            subtitle_lowest = subtitle->hits;
            subtitle_lowest_id = subtitle->id;
        }

        // pick the track with fewest forced hits
        if (subtitle->forced_hits > 0 &&
            (subtitle_forced_hits == 0 ||
             subtitle_forced_hits > subtitle->forced_hits))
        {
            subtitle_forced_id = subtitle->id;
            subtitle_forced_hits = subtitle->forced_hits;
        }
    }

    if (subtitle_forced_id && job->select_subtitle_config.force)
    {
        // If there is a subtitle stream with forced subtitles and forced-only
        // is set, then select it in preference to the lowest.
        subtitle_hit = subtitle_forced_id;
        hb_log("Found a subtitle candidate with id 0x%x (contains forced subs)",
               subtitle_hit );
    }
    else if (subtitle_lowest > 0 && subtitle_lowest < subtitle_highest * 0.1)
    {
        // OK we have more than one, and the lowest is lower,
        // but how much lower to qualify for turning it on by
        // default?
        //
        // Let's say 10% as a default.
        subtitle_hit = subtitle_lowest_id;
        hb_log( "Found a subtitle candidate with id 0x%x", subtitle_hit );
    }
    else
    {
        hb_log( "No candidate detected during subtitle scan" );
    }

    for (i = 0; i < hb_list_count( job->list_subtitle ); i++)
    {
        subtitle = hb_list_item( job->list_subtitle, i );
        if (subtitle->id == subtitle_hit)
        {
            hb_interjob_t *interjob = hb_interjob_get(job->h);

            subtitle->config = job->select_subtitle_config;
            // Remove from list since we are taking ownership
            // of the subtitle.
            hb_list_rem(job->list_subtitle, subtitle);
            interjob->select_subtitle = subtitle;
            break;
        }
    }
}

static int sanitize_subtitles( hb_job_t * job )
{
    int             i;
    uint8_t         one_burned = 0;
    hb_interjob_t * interjob = hb_interjob_get(job->h);
    hb_subtitle_t * subtitle;

    if (job->indepth_scan)
    {
        // Subtitles are set by hb_add() during subtitle scan
        return 0;
    }

    /* Look for the scanned subtitle in the existing subtitle list
     * select_subtitle implies that we did a scan. */
    if (interjob->select_subtitle != NULL)
    {
        /* Disable forced subtitles if we didn't find any in the scan, so that
         * we display normal subtitles instead. */
        if( interjob->select_subtitle->config.force &&
            interjob->select_subtitle->forced_hits == 0 )
        {
            interjob->select_subtitle->config.force = 0;
        }
        for (i = 0; i < hb_list_count( job->list_subtitle );)
        {
            subtitle = hb_list_item(job->list_subtitle, i);
            /* Remove the scanned subtitle from the list if
             * it would result in:
             * - an empty track (forced and no forced hits)
             * - an identical, duplicate subtitle track:
             *   -> both (or neither) are forced
             *   -> subtitle is not forced but all its hits are forced */
            if( ( interjob->select_subtitle->id == subtitle->id ) &&
                ( ( subtitle->config.force &&
                    interjob->select_subtitle->forced_hits == 0 ) ||
                  ( subtitle->config.force == interjob->select_subtitle->config.force ) ||
                  ( !subtitle->config.force &&
                    interjob->select_subtitle->hits == interjob->select_subtitle->forced_hits ) ) )
            {
                hb_list_rem( job->list_subtitle, subtitle );
                free( subtitle );
                continue;
            }
            /* Adjust output track number, in case we removed one.
             * Output tracks sadly still need to be in sequential order.
             * Note: out.track starts at 1, i starts at 0, and track 1 is
             * interjob->select_subtitle */
            subtitle->out_track = ++i + 1;
        }

        /* Add the subtitle that we found on the subtitle scan pass.
         *
         * Make sure it's the first subtitle in the list so that it becomes the
         * first burned subtitle (explicitly or after sanitizing) - which should
         * ensure that it doesn't get dropped. */
        interjob->select_subtitle->out_track = 1;
        if (job->pass_id == HB_PASS_ENCODE ||
            job->pass_id == HB_PASS_ENCODE_FINAL)
        {
            // final pass, interjob->select_subtitle is no longer needed
            hb_list_insert(job->list_subtitle, 0, interjob->select_subtitle);
            interjob->select_subtitle = NULL;
        }
        else
        {
            // this is not the final pass, so we need to copy it instead
            hb_list_insert(job->list_subtitle, 0, hb_subtitle_copy(interjob->select_subtitle));
        }
    }

    for (i = 0; i < hb_list_count(job->list_subtitle);)
    {
        subtitle = hb_list_item(job->list_subtitle, i);
        if (subtitle->config.dest == RENDERSUB)
        {
            if (one_burned)
            {
                if (!hb_subtitle_can_pass(subtitle->source, job->mux))
                {
                    hb_log( "More than one subtitle burn-in requested, dropping track %d.", i );
                    hb_list_rem(job->list_subtitle, subtitle);
                    free(subtitle);
                    continue;
                }
                else
                {
                    hb_log("More than one subtitle burn-in requested.  Changing track %d to soft subtitle.", i);
                    subtitle->config.dest = PASSTHRUSUB;
                }
            }
            else if (!hb_subtitle_can_burn(subtitle->source))
            {
                hb_log("Subtitle burn-in requested and input track can not be rendered.  Changing track %d to soft subtitle.", i);
                subtitle->config.dest = PASSTHRUSUB;
            }
            else
            {
                one_burned = 1;
            }
        }

        if (subtitle->config.dest == PASSTHRUSUB &&
            !hb_subtitle_can_pass(subtitle->source, job->mux))
        {
            if (!one_burned)
            {
                hb_log("Subtitle pass-thru requested and input track is not compatible with container.  Changing track %d to burned-in subtitle.", i);
                subtitle->config.dest = RENDERSUB;
                subtitle->config.default_track = 0;
                one_burned = 1;
            }
            else
            {
                hb_log("Subtitle pass-thru requested and input track is not compatible with container.  One track already burned, dropping track %d.", i);
                hb_list_rem(job->list_subtitle, subtitle);
                free(subtitle);
                continue;
            }
        }
        /* Adjust output track number, in case we removed one.
         * Output tracks sadly still need to be in sequential order.
         * Note: out.track starts at 1, i starts at 0 */
        subtitle->out_track = ++i;
    }
    if (one_burned)
    {
        // Add subtitle rendering filter
        // Note that if the filter is already in the filter chain, this
        // has no effect. Note also that this means the front-end is
        // not required to add the subtitle rendering filter since
        // we will always try to do it here.
        hb_filter_object_t *filter = hb_filter_init(HB_FILTER_RENDER_SUB);
        hb_add_filter_dict(job, filter, NULL);
    }

    return 0;
}

static int sanitize_audio(hb_job_t *job)
{
    int          i;
    hb_audio_t * audio;

    if (job->indepth_scan)
    {
        // Audio is not processed during subtitle scan
        return 0;
    }

    // apply Auto Passthru settings
    hb_autopassthru_apply_settings(job);

    for (i = 0; i < hb_list_count(job->list_audio);)
    {
        audio = hb_list_item(job->list_audio, i);
        if (audio->config.out.codec == HB_ACODEC_AUTO_PASS)
        {
            // Auto Passthru should have been handled above
            // remove track to avoid a crash
            hb_log("Auto Passthru error, dropping track %d",
                   audio->config.out.track);
            hb_list_rem(job->list_audio, audio);
            free(audio);
            continue;
        }
        if ((audio->config.out.codec & HB_ACODEC_PASS_FLAG) &&
            !(audio->config.in.codec &
              audio->config.out.codec & HB_ACODEC_PASS_MASK))
        {
            hb_log("Passthru requested and input codec is not the same as output codec for track %d, dropping track",
                   audio->config.out.track);
            hb_list_rem(job->list_audio, audio);
            free(audio);
            continue;
        }
        /*
         * never properly tested w/resampling
         * causes HandBrake GitHub issue #3533
         */
        if (audio->config.out.mixdown == HB_AMIXDOWN_RIGHT ||
            audio->config.out.mixdown == HB_AMIXDOWN_LEFT)
        {
            if (audio->config.in.samplerate !=
                hb_audio_samplerate_find_closest(audio->config.in.samplerate,
                                                 audio->config.out.codec))
            {
                // e.g. >48 kHz input, encoder w/out >48 kHz support (currently all encoders, libhb limitation)
                hb_log("work: unsupported samplerate %d for mixdown %s, dropping track %d",
                       audio->config.in.samplerate,
                       hb_mixdown_get_name(audio->config.out.mixdown),
                       audio->config.out.track);
                hb_list_rem(job->list_audio, audio);
                free(audio);
                continue;
            }
            if (audio->config.out.samplerate > 0 &&
                audio->config.out.samplerate != audio->config.in.samplerate)
            {
                // only log if specific samplerate was requested (i.e. not automatic)
                hb_log("work: sanitizing track %d samplerate %d to %d for mixdown %s",
                       audio->config.out.track,
                       audio->config.out.samplerate,
                       audio->config.in.samplerate,
                       hb_mixdown_get_name(audio->config.out.mixdown));
            }
            audio->config.out.samplerate = audio->config.in.samplerate; // no resampling
        }
        /* Adjust output track number, in case we removed one.
         * Output tracks sadly still need to be in sequential order.
         * Note: out.track starts at 1, i starts at 0 */
        audio->config.out.track = ++i;
    }

    int best_mixdown    = 0;
    int best_bitrate    = 0;
    int best_samplerate = 0;

    for (i = 0; i < hb_list_count(job->list_audio); i++)
    {
        audio = hb_list_item(job->list_audio, i);

        /* Passthru audio */
        if (audio->config.out.codec & HB_ACODEC_PASS_FLAG)
        {
            // Muxer needs these to be set correctly in order to
            // set audio track MP4 time base.
            audio->config.out.samples_per_frame =
                                    audio->config.in.samples_per_frame;
            audio->config.out.samplerate = audio->config.in.samplerate;
            continue;
        }

        /* sense-check the requested samplerate */
        if (audio->config.out.samplerate <= 0)
        {
            // if not specified, set to same as input
            audio->config.out.samplerate = audio->config.in.samplerate;
        }
        best_samplerate =
            hb_audio_samplerate_find_closest(audio->config.out.samplerate,
                                             audio->config.out.codec);
        if (best_samplerate != audio->config.out.samplerate)
        {
            hb_log("work: sanitizing track %d unsupported samplerate %d Hz to %s kHz",
                   audio->config.out.track, audio->config.out.samplerate,
                   hb_audio_samplerate_get_name(best_samplerate));
            audio->config.out.samplerate = best_samplerate;
        }

        /* sense-check the requested mixdown */
        if (audio->config.out.mixdown <= HB_AMIXDOWN_NONE)
        {
            /* Mixdown not specified, set the default mixdown */
            audio->config.out.mixdown =
                hb_mixdown_get_default(audio->config.out.codec,
                                       audio->config.in.channel_layout);
            hb_log("work: mixdown not specified, track %d setting mixdown %s",
                   audio->config.out.track,
                   hb_mixdown_get_name(audio->config.out.mixdown));
        }
        else
        {
            best_mixdown =
                hb_mixdown_get_best(audio->config.out.codec,
                                    audio->config.in.channel_layout,
                                    audio->config.out.mixdown);
            if (audio->config.out.mixdown != best_mixdown)
            {
                /* log the output mixdown */
                hb_log("work: sanitizing track %d mixdown %s to %s",
                       audio->config.out.track,
                       hb_mixdown_get_name(audio->config.out.mixdown),
                       hb_mixdown_get_name(best_mixdown));
                audio->config.out.mixdown = best_mixdown;
            }
        }

        /* sense-check the requested compression level */
        if (audio->config.out.compression_level < 0)
        {
            audio->config.out.compression_level =
                hb_audio_compression_get_default(audio->config.out.codec);
            if (audio->config.out.compression_level >= 0)
            {
                hb_log("work: compression level not specified, track %d setting compression level %.2f",
                       audio->config.out.track,
                       audio->config.out.compression_level);
            }
        }
        else
        {
            float best_compression =
                hb_audio_compression_get_best(audio->config.out.codec,
                                              audio->config.out.compression_level);
            if (best_compression != audio->config.out.compression_level)
            {
                if (best_compression == -1)
                {
                    hb_log("work: track %d, compression level not supported by codec",
                           audio->config.out.track);
                }
                else
                {
                    hb_log("work: sanitizing track %d compression level %.2f to %.2f",
                           audio->config.out.track,
                           audio->config.out.compression_level,
                           best_compression);
                }
                audio->config.out.compression_level = best_compression;
            }
        }

        /* sense-check the requested quality */
        if (audio->config.out.quality != HB_INVALID_AUDIO_QUALITY)
        {
            float best_quality =
                hb_audio_quality_get_best(audio->config.out.codec,
                                          audio->config.out.quality);
            if (best_quality != audio->config.out.quality)
            {
                if (best_quality == HB_INVALID_AUDIO_QUALITY)
                {
                    hb_log("work: track %d, quality mode not supported by codec",
                           audio->config.out.track);
                }
                else
                {
                    hb_log("work: sanitizing track %d quality %.2f to %.2f",
                           audio->config.out.track,
                           audio->config.out.quality, best_quality);
                }
                audio->config.out.quality = best_quality;
            }
        }

        /* sense-check the requested bitrate */
        if (audio->config.out.quality == HB_INVALID_AUDIO_QUALITY)
        {
            if (audio->config.out.bitrate <= 0)
            {
                /* Bitrate not specified, set the default bitrate */
                audio->config.out.bitrate =
                    hb_audio_bitrate_get_default(audio->config.out.codec,
                                                 audio->config.out.samplerate,
                                                 audio->config.out.mixdown);
                if (audio->config.out.bitrate > 0)
                {
                    hb_log("work: bitrate not specified, track %d setting bitrate %d Kbps",
                           audio->config.out.track,
                           audio->config.out.bitrate);
                }
            }
            else
            {
                best_bitrate =
                    hb_audio_bitrate_get_best(audio->config.out.codec,
                                              audio->config.out.bitrate,
                                              audio->config.out.samplerate,
                                              audio->config.out.mixdown);
                if (best_bitrate > 0 &&
                    best_bitrate != audio->config.out.bitrate)
                {
                    /* log the output bitrate */
                    hb_log("work: sanitizing track %d bitrate %d to %d Kbps",
                           audio->config.out.track,
                           audio->config.out.bitrate, best_bitrate);
                }
                audio->config.out.bitrate = best_bitrate;
            }
        }

        /* sense-check the requested dither */
        if (hb_audio_dither_is_supported(audio->config.out.codec,
                                         audio->config.in.sample_bit_depth))
        {
            if (audio->config.out.dither_method ==
                hb_audio_dither_get_default())
            {
                /* "auto", enable with default settings */
                audio->config.out.dither_method =
                    hb_audio_dither_get_default_method();
            }
        }
        else if (audio->config.out.dither_method !=
                 hb_audio_dither_get_default())
        {
            /* specific dither requested but dithering not supported */
            hb_log("work: track %d, dithering not supported by codec",
                   audio->config.out.track);
        }
    }
    return 0;
}

static void sanitize_filter_list_pre(hb_job_t *job, hb_geometry_t src_geo)
{
    hb_list_t *list = job->list_filter;

    // Add selective deinterlacing mode if comb detection is enabled
    if (hb_filter_find(list, HB_FILTER_COMB_DETECT) != NULL)
    {
        int selective[] = {HB_FILTER_DECOMB, HB_FILTER_YADIF, HB_FILTER_BWDIF};
        int ii, count = sizeof(selective) / sizeof(int);

        for (ii = 0; ii < count; ii++)
        {
            hb_filter_object_t * filter = hb_filter_find(list, selective[ii]);
            if (filter != NULL)
            {
                int mode = hb_dict_get_int(filter->settings, "mode");
                mode |= MODE_DECOMB_SELECTIVE;
                hb_dict_set(filter->settings, "mode", hb_value_int(mode));
                break;
            }
        }
    }

    int angle = 0;
    hb_filter_object_t *filter = hb_filter_find(list, HB_FILTER_ROTATE);
    if (filter != NULL)
    {
        hb_dict_t *settings = filter->settings;
        if (settings != NULL)
        {
            angle = hb_dict_get_int(settings, "angle");
        }
    }

    filter = hb_filter_find(list, HB_FILTER_CROP_SCALE);
    if (filter != NULL)
    {
        hb_dict_t *settings = filter->settings;
        if (settings != NULL)
        {
            int width, height, top, bottom, left, right;
            width = hb_dict_get_int(settings, "width");
            height = hb_dict_get_int(settings, "height");
            top = hb_dict_get_int(settings, "crop-top");
            bottom = hb_dict_get_int(settings, "crop-bottom");
            left = hb_dict_get_int(settings, "crop-left");
            right = hb_dict_get_int(settings, "crop-right");

            if (angle == 90 || angle == 270)
            {
                int temp = width;
                width = height;
                height = temp;
            }

            if (src_geo.width == width && src_geo.height == height &&
                top == 0 && bottom == 0 && left == 0 && right == 0)
            {
                hb_list_rem(list, filter);
                hb_filter_close(&filter);
                hb_log("work: skipping crop/scale filter");
            }
        }
    }

#if HB_PROJECT_FEATURE_QSV && (defined( _WIN32 ) || defined( __MINGW32__ ))
    if (hb_qsv_is_enabled(job))
    {
        hb_qsv_sanitize_filter_list(job);
    }
#endif
}

static void sanitize_filter_list_post(hb_job_t *job)
{
#ifdef __APPLE__
    if (job->hw_pix_fmt == AV_PIX_FMT_VIDEOTOOLBOX)
    {
        hb_vt_setup_hw_filters(job);
    }
#endif

    if ((job->hw_pix_fmt == AV_PIX_FMT_NONE || job->hw_pix_fmt == AV_PIX_FMT_QSV) &&
        hb_video_encoder_pix_fmt_is_supported(job->vcodec, job->input_pix_fmt, job->encoder_profile) == 0)
    {
        // Some encoders require a specific input pixel format
        // that could be different from the current pipeline format.
        const int *encoder_pix_fmts = hb_video_encoder_get_pix_fmts(job->vcodec, job->encoder_profile);
        int encoder_pix_fmt = *encoder_pix_fmts;

        // Prefer a pixel format with the
        // same chroma subsampling
        while (*encoder_pix_fmts != AV_PIX_FMT_NONE)
        {
            const AVPixFmtDescriptor *input_desc = av_pix_fmt_desc_get(job->input_pix_fmt);
            const AVPixFmtDescriptor *pix_fmt_desc = av_pix_fmt_desc_get(*encoder_pix_fmts);

            if (pix_fmt_desc->log2_chroma_w >= input_desc->log2_chroma_w &&
                pix_fmt_desc->log2_chroma_h >= input_desc->log2_chroma_h)
            {
                encoder_pix_fmt = *encoder_pix_fmts;
                break;
            }
            encoder_pix_fmts++;
        }

        hb_filter_object_t *filter = hb_filter_init(HB_FILTER_FORMAT);
        char *settings = hb_strdup_printf("format=%s", av_get_pix_fmt_name(encoder_pix_fmt));
        hb_add_filter(job, filter, settings);
        free(settings);
    }
}

static void update_dolby_vision_level(hb_job_t *job)
{
    // Dolby Vision has got its own definition of "level"
    // defined in section 2.2 of "Dolby Vision Profiles and Levels"
    // moreover, x265 requires vbv to be set, so do a rough guess here.
    // Encoders will override it when needed.
    int pps = (double)job->width * job->height * (job->vrate.num / job->vrate.den);
    int bitrate = job->vquality == HB_INVALID_VIDEO_QUALITY ? job->vbitrate : -1;
    int max_rate = hb_dovi_max_rate(job->width, pps, bitrate, 0, 1);
    job->dovi.dv_level = hb_dovi_level(job->width, pps, max_rate, 1);
}

static void sanitize_dynamic_hdr_metadata_passthru(hb_job_t *job)
{
    hb_list_t *list = job->list_filter;

    if (hb_filter_find(list, HB_FILTER_ROTATE)     != NULL ||
        hb_filter_find(list, HB_FILTER_COLORSPACE) != NULL)
    {
        job->passthru_dynamic_hdr_metadata = NONE;
        return;
    }

    if (job->vcodec != HB_VCODEC_X265_10BIT    &&
        job->vcodec != HB_VCODEC_VT_H265_10BIT &&
        job->vcodec != HB_VCODEC_SVT_AV1_10BIT)
    {
        job->passthru_dynamic_hdr_metadata &= ~HDR_10_PLUS;
    }

#if HB_PROJECT_FEATURE_LIBDOVI
    if ((job->dovi.dv_profile != 5 &&
         job->dovi.dv_profile != 7 &&
         job->dovi.dv_profile != 8) ||
        (job->vcodec != HB_VCODEC_X265_10BIT &&
         job->vcodec != HB_VCODEC_VT_H265_10BIT))
    {
        job->passthru_dynamic_hdr_metadata &= ~DOVI;
    }

    if (job->passthru_dynamic_hdr_metadata & DOVI)
    {
        int mode = 0;

        if (job->dovi.dv_profile == 7 ||
            (job->dovi.dv_profile == 8 && job->dovi.dv_bl_signal_compatibility_id == 6))
        {
            // Convert to 8.1
            mode |= 2;

            job->dovi.dv_profile = 8;
            job->dovi.el_present_flag = 0;
            job->dovi.dv_bl_signal_compatibility_id = 1;
        }

        if (hb_filter_find(list, HB_FILTER_CROP_SCALE) != NULL ||
            hb_filter_find(list, HB_FILTER_PAD)        != NULL)
        {
            // Set the active area
            mode |= 1;
        }

        double scale_factor_x = 1, scale_factor_y = 1;
        int crop_top = 0, crop_bottom = 0, crop_left = 0, crop_right = 0;
        int pad_top = 0, pad_bottom = 0, pad_left = 0, pad_right = 0;

        hb_filter_object_t *filter = hb_filter_find(list, HB_FILTER_CROP_SCALE);
        if (filter != NULL)
        {
            hb_dict_t *settings = filter->settings;
            if (settings != NULL)
            {
                int width  = hb_dict_get_int(settings, "width");
                int height = hb_dict_get_int(settings, "height");
                crop_top    = hb_dict_get_int(settings, "crop-top");
                crop_bottom = hb_dict_get_int(settings, "crop-bottom");
                crop_left   = hb_dict_get_int(settings, "crop-left");
                crop_right  = hb_dict_get_int(settings, "crop-right");

                scale_factor_x = (float)(job->title->geometry.width - crop_right - crop_left) / width;
                scale_factor_y = (float)(job->title->geometry.height - crop_top - crop_bottom) / height;
            }
        }

        filter = hb_filter_find(list, HB_FILTER_PAD);
        if (filter != NULL)
        {
            hb_dict_t *settings = filter->settings;
            if (settings != NULL)
            {
                pad_top    = hb_dict_get_int(settings, "top");
                pad_bottom = hb_dict_get_int(settings, "bottom");
                pad_left   = hb_dict_get_int(settings, "left");
                pad_right  = hb_dict_get_int(settings, "right");
            }
        }

        filter = hb_filter_init(HB_FILTER_RPU);
        char *settings = hb_strdup_printf("mode=%d:scale-factor-x=%f:scale-factor-y=%f:"
                                          "crop-top=%d:crop-bottom=%d:crop-left=%d:crop-right=%d:"
                                          "pad-top=%d:pad-bottom=%d:pad-left=%d:pad-right=%d",
                                          mode, scale_factor_x, scale_factor_y,
                                          crop_top, crop_bottom, crop_left, crop_right,
                                          pad_top, pad_bottom, pad_left, pad_right);
        hb_add_filter(job, filter, settings);
        free(settings);
    }
#else
    job->passthru_dynamic_hdr_metadata &= ~DOVI;
#endif
}

/**
 * Job initialization routine.
 *
 * Initializes fifos.
 * Creates work objects for synchronizer, video decoder, video renderer,
 * video decoder, audio decoder, audio encoder, reader, muxer.
 * Launches thread for each work object with work_loop.
 * Waits for completion of last work object.
 * Closes threads and frees fifos.
 * @param job Handle work hb_job_t.
 */
static void do_job(hb_job_t *job)
{
    int                i, result;
    hb_title_t       * title;
    hb_interjob_t    * interjob;
    hb_work_object_t * w;
    hb_audio_t       * audio;
    hb_subtitle_t    * subtitle;

    title = job->title;

    interjob = hb_interjob_get(job->h);
    if (job->sequence_id != interjob->sequence_id)
    {
        // New job sequence, clear interjob
        hb_subtitle_close(&interjob->select_subtitle);
        memset(interjob, 0, sizeof(*interjob));
        interjob->sequence_id = job->sequence_id;
    }

    job->list_work = hb_list_init();
    w = hb_get_work(job->h, WORK_READER);
    hb_list_add(job->list_work, w);

    if (job->indepth_scan)
    {
        hb_log( "Starting Task: Subtitle Scan" );
    }
    else if (job->pass_id == HB_PASS_ENCODE_ANALYSIS)
    {
        hb_log( "Starting Task: Analysis Pass" );
    }
    else
    {
        hb_log( "Starting Task: Encoding Pass" );
    }

    // Allow the usage of the hardware decoder
    // only if it was marked as supported in the scan
    // TODO: remove the ifdef after WinUI is updated
#ifdef __APPLE__
    if ((title->video_decode_support & job->hw_decode) == 0)
    {
        job->hw_decode = 0;
    }
#endif

    // This must be performed before initializing filters because
    // it can add the subtitle render filter.
    result = sanitize_subtitles(job);
    if (result)
    {
        *job->done_error = HB_ERROR_WRONG_INPUT;
        *job->die = 1;
        goto cleanup;
    }
    // Filters have an effect on settings.
    // So initialize the filters and update the job.
    if (job->list_filter && hb_list_count(job->list_filter))
    {
        hb_filter_init_t init;

        sanitize_filter_list_pre(job, title->geometry);

        // Select the optimal pixel formats for the pipeline
        job->hw_pix_fmt = hb_get_best_hw_pix_fmt(job);
        job->input_pix_fmt = hb_get_best_pix_fmt(job);

        // Init hwaccel context if needed
        if (hb_hwaccel_decode_is_enabled(job))
        {
            hb_hwaccel_hw_ctx_init(job->title->video_codec_param,
                                   job->hw_decode,
                                   &job->hw_device_ctx);
        }

        sanitize_dynamic_hdr_metadata_passthru(job);
        sanitize_filter_list_post(job);

        memset(&init, 0, sizeof(init));
        init.time_base.num = 1;
        init.time_base.den = 90000;
        init.job = job;
        init.pix_fmt = job->input_pix_fmt;
        init.hw_pix_fmt = job->hw_pix_fmt;

        init.color_prim = title->color_prim;
        init.color_transfer = title->color_transfer;
        init.color_matrix = title->color_matrix;
        // Dolby Vision profile 5 requires full range
        // TODO: find a better way to handle this
        init.color_range = job->passthru_dynamic_hdr_metadata & DOVI &&
                            job->dovi.dv_profile == 5 ?
                            title->color_range : AVCOL_RANGE_MPEG;
#if HB_PROJECT_FEATURE_QSV
        if (hb_qsv_full_path_is_enabled(job))
        {
            init.color_range = (job->qsv.ctx->out_range == AVCOL_RANGE_UNSPECIFIED) ? title->color_range : job->qsv.ctx->out_range;
        }
#endif
        init.chroma_location = title->chroma_location;
        init.geometry = title->geometry;
        memset(init.crop, 0, sizeof(int[4]));
        init.vrate = job->vrate;
        init.cfr = 0;
        init.grayscale = 0;

        for( i = 0; i < hb_list_count( job->list_filter ); )
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );
            filter->done = &job->done;
            if (filter->init != NULL && filter->init(filter, &init))
            {
                hb_log( "Failure to initialise filter '%s', disabling",
                        filter->name );
                hb_list_rem( job->list_filter, filter );
                hb_filter_close( &filter );
                continue;
            }
            i++;
        }
        job->output_pix_fmt = init.pix_fmt;
        job->color_prim = init.color_prim;
        job->color_transfer = init.color_transfer;
        job->color_matrix = init.color_matrix;
        job->color_range = init.color_range;
        job->chroma_location = init.chroma_location;
        job->width = init.geometry.width;
        job->height = init.geometry.height;
        // job->par is supplied by the frontend.
        //
        // The filter chain does not know what the final desired PAR is.
        // job->par = init.geometry.par;
        memcpy(job->crop, init.crop, sizeof(int[4]));
        job->vrate = init.vrate;
        job->cfr = init.cfr;
        job->grayscale = init.grayscale;

        // Combine HB_FILTER_AVFILTERs that are sequential
        hb_avfilter_combine(job->list_filter);

        // Perform filter post_init which informs filters of final
        // job configuration. e.g. rendersub filter needs to know the
        // final crop dimensions.
        for( i = 0; i < hb_list_count( job->list_filter ); )
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );
            filter->done = &job->done;
            if (filter->post_init != NULL && filter->post_init(filter, job))
            {
                hb_log( "Failure to initialise filter '%s', disabling",
                        filter->name );
                hb_list_rem( job->list_filter, filter );
                hb_filter_close( &filter );
                continue;
            }
            i++;
        }
    }
    else
    {
        job->width = title->geometry.width;
        job->height = title->geometry.height;
        job->par = title->geometry.par;
        memset(job->crop, 0, sizeof(int[4]));
        job->vrate = title->vrate;
        job->cfr = 0;
    }

    job->orig_vrate = job->vrate;
    if (job->pass_id == HB_PASS_ENCODE_FINAL)
    {
        correct_framerate(interjob, job);
    }

    /*
     * The frame rate may affect the bitstream's time base, lose superfluous
     * factors for consistency (some encoders reduce fractions, some don't).
     */
    hb_reduce(&job->orig_vrate.num, &job->orig_vrate.den,
               job->orig_vrate.num,  job->orig_vrate.den);
    hb_reduce(&job->vrate.num, &job->vrate.den,
               job->vrate.num,  job->vrate.den);

    if (job->passthru_dynamic_hdr_metadata & DOVI)
    {
        // Dolby Vision level needs to be updated now that
        // the final width, height and frame rate is known
        update_dolby_vision_level(job);
    }

    job->fifo_mpeg2  = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
    job->fifo_raw    = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
    if (!job->indepth_scan)
    {
        // When doing subtitle indepth scan, the pipeline ends at sync
        job->fifo_sync   = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
        job->fifo_render = NULL; // Attached to filter chain
        job->fifo_mpeg4  = hb_fifo_init( FIFO_LARGE, FIFO_LARGE_WAKE );
    }

    result = sanitize_audio(job);
    if (result)
    {
        *job->done_error = HB_ERROR_WRONG_INPUT;
        *job->die = 1;
        goto cleanup;
    }

    if (!job->indepth_scan)
    {
        // Set up audio decoder work objects
        // Audio fifos must be initialized before sync
        for (i = 0; i < hb_list_count(job->list_audio); i++)
        {
            audio = hb_list_item(job->list_audio, i);

            /* set up the audio work fifos */
            audio->priv.fifo_in   = hb_fifo_init(FIFO_LARGE, FIFO_LARGE_WAKE);
            audio->priv.fifo_raw  = hb_fifo_init(FIFO_SMALL, FIFO_SMALL_WAKE);
            audio->priv.fifo_sync = hb_fifo_init(FIFO_SMALL, FIFO_SMALL_WAKE);
            audio->priv.fifo_out  = hb_fifo_init(FIFO_LARGE, FIFO_LARGE_WAKE);

            // Add audio decoder work object
            w = hb_audio_decoder(job->h, audio->config.in.codec);
            if (w == NULL)
            {
                hb_error("Invalid input codec: %d", audio->config.in.codec);
                *job->done_error = HB_ERROR_WRONG_INPUT;
                *job->die = 1;
                goto cleanup;
            }
            w->init_delay = &audio->priv.init_delay;
            w->extradata  = &audio->priv.extradata;
            w->fifo_in  = audio->priv.fifo_in;
            w->fifo_out = audio->priv.fifo_raw;
            w->audio    = audio;
            w->codec_param = audio->config.in.codec_param;

            hb_list_add( job->list_work, w );
        }
    }

    // Subtitle fifos must be initialized before sync
    for (i = 0; i < hb_list_count( job->list_subtitle ); i++)
    {
        subtitle = hb_list_item( job->list_subtitle, i );
        w = hb_get_work( job->h, subtitle->codec );
        // Must set capacity of the raw-FIFO to be set >= the maximum
        // number of subtitle lines that could be decoded prior to a
        // video frame in order to prevent the following deadlock
        // condition:
        //   1. Subtitle decoder blocks trying to generate more subtitle
        //      lines than will fit in the FIFO.
        //   2. Blocks the processing of further subtitle packets read
        //      from the input stream.
        //   3. And that blocks the processing of any further video
        //      packets read from the input stream.
        //   4. And that blocks the sync work-object from running, which
        //      is needed to consume the subtitle lines in the raw-FIFO.
        // Since that number is unbounded, the FIFO must be made
        // (effectively) unbounded in capacity.
        subtitle->fifo_raw  = hb_fifo_init( FIFO_UNBOUNDED, FIFO_UNBOUNDED_WAKE );
        // Check if input comes from a file.
        if (subtitle->source != IMPORTSRT &&
            subtitle->source != IMPORTSSA)
        {
            subtitle->fifo_in  = hb_fifo_init( FIFO_UNBOUNDED, FIFO_UNBOUNDED_WAKE );
        }
        if (!job->indepth_scan)
        {
            // When doing subtitle indepth scan, the pipeline ends at sync
            subtitle->fifo_out = hb_fifo_init( FIFO_UNBOUNDED, FIFO_UNBOUNDED_WAKE );
        }

        w->fifo_in = subtitle->fifo_in;
        w->fifo_out = subtitle->fifo_raw;
        w->subtitle = subtitle;
        hb_list_add( job->list_work, w );
    }

    // Video decoder
    w = hb_video_decoder(job->h, title->video_codec, title->video_codec_param, job->hw_device_ctx);
    if (w == NULL)
    {
        *job->done_error = HB_ERROR_WRONG_INPUT;
        *job->die = 1;
        goto cleanup;
    }
    w->fifo_in  = job->fifo_mpeg2;
    w->fifo_out = job->fifo_raw;
    hb_list_add(job->list_work, w);

    // Synchronization
    w = hb_get_work(job->h, WORK_SYNC_VIDEO);
    hb_list_add(job->list_work, w);

    if (!job->indepth_scan)
    {
        for( i = 0; i < hb_list_count( job->list_audio ); i++ )
        {
            audio = hb_list_item( job->list_audio, i );

            /*
            * Audio Encoder Thread
            */
            if ( !(audio->config.out.codec & HB_ACODEC_PASS_FLAG ) )
            {
                /*
                * Add the encoder thread if not doing pass through
                */
                w = hb_audio_encoder( job->h, audio->config.out.codec);
                if (w == NULL)
                {
                    hb_error("Invalid audio codec: %#x", audio->config.out.codec);
                    w = NULL;
                    *job->done_error = HB_ERROR_WRONG_INPUT;
                    *job->die = 1;
                    goto cleanup;
                }
                w->init_delay = &audio->priv.init_delay;
                w->extradata  = &audio->priv.extradata;
                w->fifo_in  = audio->priv.fifo_sync;
                w->fifo_out = audio->priv.fifo_out;
                w->audio    = audio;

                hb_list_add( job->list_work, w );
            }
        }

        /* Set up the video filter fifo pipeline */
        if ( job->list_filter )
        {
            hb_fifo_t * fifo_in = job->fifo_sync;
            for (i = 0; i < hb_list_count(job->list_filter); i++)
            {
                hb_filter_object_t * filter = hb_list_item(job->list_filter, i);
                if (!filter->skip)
                {
                    filter->fifo_in = fifo_in;
                    filter->fifo_out = hb_fifo_init(FIFO_MINI, FIFO_MINI_WAKE);
                    fifo_in = filter->fifo_out;
                }
            }
            job->fifo_render = fifo_in;
        }
        else if ( !job->list_filter )
        {
            hb_log("work: Internal Error: no filters");
            job->fifo_render = NULL;
        }

        // Video encoder
        w = hb_video_encoder(job->h, job->vcodec);
        if (w == NULL)
        {
            *job->done_error = HB_ERROR_INIT;
            *job->die = 1;
            goto cleanup;
        }

        // Handle case where there are no filters.
        // This really should never happen.
        if ( job->fifo_render )
            w->fifo_in  = job->fifo_render;
        else
            w->fifo_in  = job->fifo_sync;

        w->fifo_out  =  job->fifo_mpeg4;

        w->init_delay = &job->init_delay;
        w->extradata  = &job->extradata;

        hb_list_add( job->list_work, w );

    }

    // Add Muxer work object
    // Muxer work object should be the last object added to the list
    // during regular encoding pass.  For subtitle scan, sync is last.
    if (!job->indepth_scan)
    {
        w = hb_get_work(job->h, WORK_MUX);
        hb_list_add(job->list_work, w);
    }

    if( job->chapter_markers && job->chapter_start == job->chapter_end )
    {
        job->chapter_markers = 0;
        hb_log("work: only 1 chapter, disabling chapter markers");
    }

    /* Display settings */
    hb_display_job_info( job );

    // Initialize all work objects
    job->done = 0;
    for (i = 0; i < hb_list_count( job->list_work ); i++)
    {
        w = hb_list_item( job->list_work, i );
        w->done = &job->done;
        if (w->init( w, job ))
        {
            hb_error( "Failure to initialise thread '%s'", w->name );
            *job->done_error = HB_ERROR_INIT;
            *job->die = 1;
            goto cleanup;
        }
    }

    /* Launch processing threads */
    for (i = 0; i < hb_list_count( job->list_work ); i++)
    {
        w = hb_list_item(job->list_work, i);
        w->thread = hb_thread_init(w->name, hb_work_loop, w, HB_LOW_PRIORITY);
    }
    if (job->list_filter && !job->indepth_scan)
    {
        for (i = 0; i < hb_list_count(job->list_filter); i++)
        {
            hb_filter_object_t * filter = hb_list_item(job->list_filter, i);

            if (!filter->skip)
            {
                // Filters were initialized earlier, so we just need
                // to start the filter's thread
                filter->thread = hb_thread_init(filter->name, filter_loop,
                                                filter, HB_LOW_PRIORITY);
            }
        }
    }

    // Wait for the thread of the last work object to complete
    // Note that other threads may still be running even though the
    // last thread has exited. So we must be careful with the sequence
    // of closing threads below.
    w = hb_list_item(job->list_work, hb_list_count(job->list_work) - 1);
    w->die = job->die;
    hb_thread_close(&w->thread);

    hb_handle_t * h = job->h;
    hb_state_t state;
    hb_get_state2( h, &state );

    hb_log("work: average encoding speed for job is %f fps",
           state.param.working.rate_avg);

cleanup:
    job->done = 1;

    // Close render filter pipeline
    if (job->list_filter)
    {
        for (i = 0; i < hb_list_count(job->list_filter); i++)
        {
            hb_filter_object_t * filter = hb_list_item(job->list_filter, i);
            if( filter->thread != NULL )
            {
                hb_thread_close(&filter->thread);
            }
            filter->close(filter);
        }
    }

    // Close work objects
    // A work thread can use data created by another work thread's init.
    // So close all work threads before closing thread data.
    for (i = 0; i < hb_list_count(job->list_work); i++)
    {
        w = hb_list_item(job->list_work, i);
        if (w->thread != NULL)
        {
            hb_thread_close(&w->thread);
        }
    }
    while ((w = hb_list_item(job->list_work, 0)))
    {
        hb_list_rem(job->list_work, w);
        w->close(w);
        free(w);
    }

    hb_list_close( &job->list_work );

    /* Close fifos */
    hb_fifo_close( &job->fifo_mpeg2 );
    hb_fifo_close( &job->fifo_raw );
    hb_fifo_close( &job->fifo_sync );
    hb_fifo_close( &job->fifo_mpeg4 );

    for (i = 0; i < hb_list_count( job->list_subtitle ); i++)
    {
        subtitle = hb_list_item( job->list_subtitle, i );
        if( subtitle )
        {
            hb_fifo_close( &subtitle->fifo_in );
            hb_fifo_close( &subtitle->fifo_raw );
            hb_fifo_close( &subtitle->fifo_out );
        }
    }
    for (i = 0; i < hb_list_count( job->list_audio ); i++)
    {
        audio = hb_list_item( job->list_audio, i );
        if( audio->priv.fifo_in != NULL )
            hb_fifo_close( &audio->priv.fifo_in );
        if( audio->priv.fifo_raw != NULL )
            hb_fifo_close( &audio->priv.fifo_raw );
        if( audio->priv.fifo_sync != NULL )
            hb_fifo_close( &audio->priv.fifo_sync );
        if( audio->priv.fifo_out != NULL )
            hb_fifo_close( &audio->priv.fifo_out );
    }

    if (job->list_filter)
    {
        for (i = 0; i < hb_list_count( job->list_filter ); i++)
        {
            hb_filter_object_t * filter = hb_list_item( job->list_filter, i );
            if (!filter->skip)
            {
                hb_fifo_close( &filter->fifo_out );
            }
        }
    }

    if (job->indepth_scan)
    {
        analyze_subtitle_scan(job);
    }

    hb_buffer_pool_free();
    hb_hwaccel_hw_ctx_close(&job->hw_device_ctx);

#if HB_PROJECT_FEATURE_QSV
    if (!job->indepth_scan &&
        (job->pass_id != HB_PASS_ENCODE_ANALYSIS) &&
        hb_qsv_is_enabled(job))
    {
        hb_qsv_context_uninit(job);
    }
#endif
}

static inline void copy_chapter( hb_buffer_t * dst, hb_buffer_t * src )
{
    // Propagate any chapter breaks for the worker if and only if the
    // output frame has the same time stamp as the input frame (any
    // worker that delays frames has to propagate the chapter marks itself
    // and workers that move chapter marks to a different time should set
    // 'src' to NULL so that this code won't generate spurious duplicates.)
    if( src && dst && src->s.start == dst->s.start && src->s.new_chap != 0)
    {
        // restore log below to debug chapter mark propagation problems
        dst->s.new_chap = src->s.new_chap;
    }
}

/**
 * Performs the work object's specific work function.
 * Loops calling work function for associated work object. Sleeps when fifo is full.
 * Monitors work done indicator.
 * Exits loop when work indicator is set.
 * @param _w Handle to work object.
 */
void hb_work_loop( void * _w )
{
    hb_work_object_t * w = _w;
    hb_buffer_t      * buf_in = NULL, * buf_out = NULL;

    while ((w->die == NULL || !*w->die) && !*w->done &&
           w->status != HB_WORK_DONE)
    {
        // fifo_in == NULL means this is a data source (e.g. reader)
        if (w->fifo_in != NULL)
        {
            buf_in = hb_fifo_get_wait( w->fifo_in );
            if ( buf_in == NULL )
                continue;
            if ( *w->done )
            {
                if( buf_in )
                {
                    hb_buffer_close( &buf_in );
                }
                break;
            }
        }
        // Invalidate buf_out so that if there is no output
        // we don't try to pass along junk.
        buf_out = NULL;
        w->status = w->work( w, &buf_in, &buf_out );

        copy_chapter( buf_out, buf_in );

        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
        if ( buf_out && w->fifo_out == NULL )
        {
            hb_buffer_close( &buf_out );
        }
        if( buf_out )
        {
            while ( !*w->done )
            {
                if ( hb_fifo_full_wait( w->fifo_out ) )
                {
                    hb_fifo_push( w->fifo_out, buf_out );
                    buf_out = NULL;
                    break;
                }
            }
        }
        else if (w->fifo_in == NULL)
        {
            // If this work object is a generator (no input fifo) and it
            // generated no output, it may be waiting for status from
            // another thread. Yield so that we don't spin doing nothing.
            hb_yield();
        }
    }
    if ( buf_out )
    {
        hb_buffer_close( &buf_out );
    }

    // Consume data in incoming fifo till job completes so that
    // residual data does not stall the pipeline. There can be
    // residual data during point-to-point encoding.
    hb_deep_log(3, "worker %s waiting to die", w->name);
    while ((w->die == NULL || !*w->die) &&
           !*w->done && w->fifo_in != NULL)
    {
        buf_in = hb_fifo_get_wait( w->fifo_in );
        if ( buf_in != NULL )
            hb_buffer_close( &buf_in );
    }
}

/**
 * Performs the filter object's specific work function.
 * Loops calling work function for associated filter object.
 * Sleeps when fifo is full.
 * Monitors work done indicator.
 * Exits loop when work indicator is set.
 * @param _w Handle to work object.
 */
static void filter_loop( void * _f )
{
    hb_filter_object_t * f = _f;
    hb_buffer_t      * buf_in, * buf_out = NULL;

    while( !*f->done && f->status != HB_FILTER_DONE )
    {
        buf_in = hb_fifo_get_wait( f->fifo_in );
        if ( buf_in == NULL )
            continue;

        // Filters can drop buffers.  Remember chapter information
        // so that it can be propagated to the next buffer
        if ( buf_in->s.new_chap )
        {
            f->chapter_time = buf_in->s.start;
            f->chapter_val = buf_in->s.new_chap;
            // don't let 'filter_loop' put a chapter mark on the wrong buffer
            buf_in->s.new_chap = 0;
        }
        if ( *f->done )
        {
            if( buf_in )
            {
                hb_buffer_close( &buf_in );
            }
            break;
        }

        buf_out = NULL;

        f->status = f->work( f, &buf_in, &buf_out );

        if ( buf_out && f->chapter_val && f->chapter_time <= buf_out->s.start )
        {
            buf_out->s.new_chap = f->chapter_val;
            f->chapter_val = 0;
        }

        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
        if ( buf_out && f->fifo_out == NULL )
        {
            hb_buffer_close( &buf_out );
        }
        if( buf_out )
        {
            while ( !*f->done )
            {
                if ( hb_fifo_full_wait( f->fifo_out ) )
                {
                    hb_fifo_push( f->fifo_out, buf_out );
                    buf_out = NULL;
                    break;
                }
            }
        }
    }
    if ( buf_out )
    {
        hb_buffer_close( &buf_out );
    }

    // Consume data in incoming fifo till job complete so that
    // residual data does not stall the pipeline
    while( !*f->done )
    {
        buf_in = hb_fifo_get_wait( f->fifo_in );
        if ( buf_in != NULL )
            hb_buffer_close( &buf_in );
    }
}

