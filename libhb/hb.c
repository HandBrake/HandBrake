/* hb.c

   Copyright (c) 2003-2013 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */
 
#include "hb.h"
#include "opencl.h"
#include "hbffmpeg.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_QSV
#include "qsv_common.h"
#endif

#if defined( SYS_MINGW )
#include <io.h>
#if defined( PTW32_STATIC_LIB )
#include <pthread.h>
#endif
#endif

struct hb_handle_s
{
    int            id;
    
    /* The "Check for update" thread */
    int            build;
    char           version[32];
    hb_thread_t  * update_thread;

    /* This thread's only purpose is to check other threads'
       states */
    volatile int   die;
    hb_thread_t  * main_thread;
    int            pid;

    /* DVD/file scan thread */
    hb_title_set_t title_set;
    hb_thread_t  * scan_thread;

    /* The thread which processes the jobs. Others threads are launched
       from this one (see work.c) */
    hb_list_t    * jobs;
    hb_job_t     * current_job;
    int            job_count;
    int            job_count_permanent;
    volatile int   work_die;
    hb_error_code  work_error;
    hb_thread_t  * work_thread;

    hb_lock_t    * state_lock;
    hb_state_t     state;

    int            paused;
    hb_lock_t    * pause_lock;
    /* For MacGui active queue
       increments each time the scan thread completes*/
    int            scanCount;
    volatile int   scan_die;
    
    /* Stash of persistent data between jobs, for stuff
       like correcting frame count and framerate estimates
       on multi-pass encodes where frames get dropped.     */
    hb_interjob_t * interjob;

    // power management opaque pointer
    void *system_sleep_opaque;
} ;

hb_work_object_t * hb_objects = NULL;
int hb_instance_counter = 0;

static void thread_func( void * );

static int ff_lockmgr_cb(void **mutex, enum AVLockOp op)
{
    switch ( op )
    {
        case AV_LOCK_CREATE:
        {
            *mutex  = hb_lock_init();
        } break;
        case AV_LOCK_DESTROY:
        {
            hb_lock_close( (hb_lock_t**)mutex );
        } break;
        case AV_LOCK_OBTAIN:
        {
            hb_lock( (hb_lock_t*)*mutex );
        } break;
        case AV_LOCK_RELEASE:
        {
            hb_unlock( (hb_lock_t*)*mutex );
        } break;
        default:
            break;
    }
    return 0;
}

void hb_avcodec_init()
{
    av_lockmgr_register(ff_lockmgr_cb);
    av_register_all();
#ifdef _WIN64
    // avresample's assembly optimizations can cause crashes under Win x86_64
    // (see http://bugzilla.libav.org/show_bug.cgi?id=496)
    // disable AVX and FMA4 as a workaround
    hb_deep_log(2, "hb_avcodec_init: Windows x86_64, disabling AVX and FMA4");
    int cpu_flags = av_get_cpu_flags() & ~AV_CPU_FLAG_AVX & ~AV_CPU_FLAG_FMA4;
    av_set_cpu_flags_mask(cpu_flags);
#endif
}

int hb_avcodec_open(AVCodecContext *avctx, AVCodec *codec,
                    AVDictionary **av_opts, int thread_count)
{
    int ret;

    if ((thread_count == HB_FFMPEG_THREADS_AUTO || thread_count > 0) && 
        (codec->type == AVMEDIA_TYPE_VIDEO))
    {
        avctx->thread_count = (thread_count == HB_FFMPEG_THREADS_AUTO) ?
                               hb_get_cpu_count() / 2 + 1 : thread_count;
        avctx->thread_type = FF_THREAD_FRAME|FF_THREAD_SLICE;
        avctx->thread_safe_callbacks = 1;
    }
    else
    {
        avctx->thread_count = 1;
    }

    if (codec->capabilities & CODEC_CAP_EXPERIMENTAL)
    {
        // "experimental" encoders will not open without this
        avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    ret = avcodec_open2(avctx, codec, av_opts);
    return ret;
}

int hb_avcodec_close(AVCodecContext *avctx)
{
    int ret;
    ret = avcodec_close(avctx);
    return ret;
}


int hb_avpicture_fill(AVPicture *pic, hb_buffer_t *buf)
{
    int ret, ii;

    for (ii = 0; ii < 4; ii++)
        pic->linesize[ii] = buf->plane[ii].stride;

    ret = av_image_fill_pointers(pic->data, buf->f.fmt,
                                 buf->plane[0].height_stride,
                                 buf->data, pic->linesize);
    if (ret != buf->size)
    {
        hb_error("Internal error hb_avpicture_fill expected %d, got %d",
                 buf->size, ret);
    }
    return ret;
}

static int handle_jpeg(enum AVPixelFormat *format)
{
    switch (*format)
    {
        case AV_PIX_FMT_YUVJ420P: *format = AV_PIX_FMT_YUV420P; return 1;
        case AV_PIX_FMT_YUVJ422P: *format = AV_PIX_FMT_YUV422P; return 1;
        case AV_PIX_FMT_YUVJ444P: *format = AV_PIX_FMT_YUV444P; return 1;
        case AV_PIX_FMT_YUVJ440P: *format = AV_PIX_FMT_YUV440P; return 1;
        default:                                                return 0;
    }
}

struct SwsContext*
hb_sws_get_context(int srcW, int srcH, enum AVPixelFormat srcFormat,
                   int dstW, int dstH, enum AVPixelFormat dstFormat,
                   int flags)
{
    struct SwsContext * ctx;

    ctx = sws_alloc_context();
    if ( ctx )
    {
        int srcRange, dstRange;

        srcRange = handle_jpeg(&srcFormat);
        dstRange = handle_jpeg(&dstFormat);
        /* enable this when implemented in Libav
        flags |= SWS_FULL_CHR_H_INT | SWS_FULL_CHR_H_INP;
         */

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
                      sws_getCoefficients( SWS_CS_DEFAULT ), // src colorspace
                      srcRange, // src range 0 = MPG, 1 = JPG
                      sws_getCoefficients( SWS_CS_DEFAULT ), // dst colorspace
                      dstRange, // dst range 0 = MPG, 1 = JPG
                      0,         // brightness
                      1 << 16,   // contrast
                      1 << 16 ); // saturation

        if (sws_init_context(ctx, NULL, NULL) < 0) {
            fprintf(stderr, "Cannot initialize resampling context\n");
            sws_freeContext(ctx);
            ctx = NULL;
        } 
    }
    return ctx;
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
void hb_ff_set_sample_fmt(AVCodecContext *context, AVCodec *codec,
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

/**
 * Registers work objects, by adding the work object to a liked list.
 * @param w Handle to hb_work_object_t to register.
 */
void hb_register( hb_work_object_t * w )
{
    w->next    = hb_objects;
    hb_objects = w;
}

void (*hb_log_callback)(const char* message);
static void redirect_thread_func(void *);

#if defined( SYS_MINGW )
#define pipe(phandles)  _pipe (phandles, 4096, _O_BINARY)
#endif

/**
 * Registers the given function as a logger. All logs will be passed to it.
 * @param log_cb The function to register as a logger.
 */
void hb_register_logger( void (*log_cb)(const char* message) )
{
    hb_log_callback = log_cb;
    hb_thread_init("ioredirect", redirect_thread_func, NULL, HB_NORMAL_PRIORITY);
}

/**
 * libhb initialization routine.
 * @param verbose HB_DEBUG_NONE or HB_DEBUG_ALL.
 * @param update_check signals libhb to check for updated version from HandBrake website.
 * @return Handle to hb_handle_t for use on all subsequent calls to libhb.
 */
hb_handle_t * hb_init( int verbose, int update_check )
{
    hb_handle_t * h = calloc( sizeof( hb_handle_t ), 1 );
    uint64_t      date;

    /* See hb_deep_log() and hb_log() in common.c */
    global_verbosity_level = verbose;
    if( verbose )
        putenv( "HB_DEBUG=1" );
    
    h->id = hb_instance_counter++;
    
    /* Check for an update on the website if asked to */
    h->build = -1;

    /* Initialize opaque for PowerManagement purposes */
    h->system_sleep_opaque = hb_system_sleep_opaque_init();

    if( update_check )
    {
        hb_log( "hb_init: checking for updates" );
        date             = hb_get_date();
        h->update_thread = hb_update_init( &h->build, h->version );

        for( ;; )
        {
            if( hb_thread_has_exited( h->update_thread ) )
            {
                /* Immediate success or failure */
                hb_thread_close( &h->update_thread );
                break;
            }
            if( hb_get_date() > date + 1000 )
            {
                /* Still nothing after one second. Connection problem,
                   let the thread die */
                hb_log( "hb_init: connection problem, not waiting for "
                        "update_thread" );
                break;
            }
            hb_snooze( 500 );
        }
    }

    /*
     * Initialise buffer pool
     */
    hb_buffer_pool_init();

    h->title_set.list_title = hb_list_init();
    h->jobs       = hb_list_init();

    h->state_lock  = hb_lock_init();
    h->state.state = HB_STATE_IDLE;

    h->pause_lock = hb_lock_init();

    h->interjob = calloc( sizeof( hb_interjob_t ), 1 );

    /* Start library thread */
    hb_log( "hb_init: starting libhb thread" );
    h->die         = 0;
    h->main_thread = hb_thread_init( "libhb", thread_func, h,
                                     HB_NORMAL_PRIORITY );
    
    return h;
}

/**
 * libhb initialization routine.
 * This version is to use when calling the dylib, the macro hb_init isn't available from a dylib call!
 * @param verbose HB_DEBUG_NONE or HB_DEBUG_ALL.
 * @param update_check signals libhb to check for updated version from HandBrake website.
 * @return Handle to hb_handle_t for use on all subsequent calls to libhb.
 */
hb_handle_t * hb_init_dl( int verbose, int update_check )
{
    hb_handle_t * h = calloc( sizeof( hb_handle_t ), 1 );
    uint64_t      date;

    /* See hb_log() in common.c */
    if( verbose > HB_DEBUG_NONE )
    {
        putenv( "HB_DEBUG=1" );
    }

    h->id = hb_instance_counter++;

    /* Check for an update on the website if asked to */
    h->build = -1;

    /* Initialize opaque for PowerManagement purposes */
    h->system_sleep_opaque = hb_system_sleep_opaque_init();

    if( update_check )
    {
        hb_log( "hb_init: checking for updates" );
        date             = hb_get_date();
        h->update_thread = hb_update_init( &h->build, h->version );

        for( ;; )
        {
            if( hb_thread_has_exited( h->update_thread ) )
            {
                /* Immediate success or failure */
                hb_thread_close( &h->update_thread );
                break;
            }
            if( hb_get_date() > date + 1000 )
            {
                /* Still nothing after one second. Connection problem,
                   let the thread die */
                hb_log( "hb_init: connection problem, not waiting for "
                        "update_thread" );
                break;
            }
            hb_snooze( 500 );
        }
    }

    h->title_set.list_title = hb_list_init();
    h->jobs       = hb_list_init();
    h->current_job = NULL;

    h->state_lock  = hb_lock_init();
    h->state.state = HB_STATE_IDLE;

    h->pause_lock = hb_lock_init();

    /* Start library thread */
    hb_log( "hb_init: starting libhb thread" );
    h->die         = 0;
    h->main_thread = hb_thread_init( "libhb", thread_func, h,
                                     HB_NORMAL_PRIORITY );

    return h;
}


/**
 * Returns current version of libhb.
 * @param h Handle to hb_handle_t.
 * @return character array of version number.
 */
char * hb_get_version( hb_handle_t * h )
{
    return HB_PROJECT_VERSION;
}

/**
 * Returns current build of libhb.
 * @param h Handle to hb_handle_t.
 * @return character array of build number.
 */
int hb_get_build( hb_handle_t * h )
{
    return HB_PROJECT_BUILD;
}

/**
 * Checks for needed update.
 * @param h Handle to hb_handle_t.
 * @param version Pointer to handle where version will be copied.
 * @return update indicator.
 */
int hb_check_update( hb_handle_t * h, char ** version )
{
    *version = ( h->build < 0 ) ? NULL : h->version;
    return h->build;
}

/**
 * Deletes current previews associated with titles
 * @param h Handle to hb_handle_t
 */
void hb_remove_previews( hb_handle_t * h )
{
    char            filename[1024];
    char            dirname[1024];
    hb_title_t    * title;
    int             i, count, len;
    DIR           * dir;
    struct dirent * entry;

    memset( dirname, 0, 1024 );
    hb_get_temporary_directory( dirname );
    dir = opendir( dirname );
    if (dir == NULL) return;

    count = hb_list_count( h->title_set.list_title );
    while( ( entry = readdir( dir ) ) )
    {
        if( entry->d_name[0] == '.' )
        {
            continue;
        }
        for( i = 0; i < count; i++ )
        {
            title = hb_list_item( h->title_set.list_title, i );
            len = snprintf( filename, 1024, "%d_%d", h->id, title->index );
            if (strncmp(entry->d_name, filename, len) == 0)
            {
                snprintf( filename, 1024, "%s/%s", dirname, entry->d_name );
                unlink( filename );
                break;
            }
        }
    }
    closedir( dir );
}

/**
 * Initializes a scan of the by calling hb_scan_init
 * @param h Handle to hb_handle_t
 * @param path location of VIDEO_TS folder.
 * @param title_index Desired title to scan.  0 for all titles.
 * @param preview_count Number of preview images to generate.
 * @param store_previews Whether or not to write previews to disk.
 */
void hb_scan( hb_handle_t * h, const char * path, int title_index,
              int preview_count, int store_previews, uint64_t min_duration )
{
    hb_title_t * title;

    h->scan_die = 0;

    /* Clean up from previous scan */
    hb_remove_previews( h );
    while( ( title = hb_list_item( h->title_set.list_title, 0 ) ) )
    {
        hb_list_rem( h->title_set.list_title, title );
        hb_title_close( &title );
    }

    /* Print CPU info here so that it's in all scan and encode logs */
    const char *cpu_name = hb_get_cpu_name();
    const char *cpu_type = hb_get_cpu_platform_name();
    hb_log("CPU: %s", cpu_name != NULL ? cpu_name : "");
    if (cpu_type != NULL)
    {
        hb_log(" - %s", cpu_type);
    }
    hb_log(" - logical processor count: %d", hb_get_cpu_count());

    /* Print OpenCL info here so that it's in all scan and encode logs */
    hb_opencl_info_print();

#ifdef USE_QSV
    /* Print QSV info here so that it's in all scan and encode logs */
    hb_qsv_info_print();
#endif

    hb_log( "hb_scan: path=%s, title_index=%d", path, title_index );
    h->scan_thread = hb_scan_init( h, &h->scan_die, path, title_index, 
                                   &h->title_set, preview_count, 
                                   store_previews, min_duration );
}

/**
 * Returns the list of titles found.
 * @param h Handle to hb_handle_t
 * @return Handle to hb_list_t of the title list.
 */
hb_list_t * hb_get_titles( hb_handle_t * h )
{
    return h->title_set.list_title;
}

hb_title_set_t * hb_get_title_set( hb_handle_t * h )
{
    return &h->title_set;
}

int hb_save_preview( hb_handle_t * h, int title, int preview, hb_buffer_t *buf )
{
    FILE * file;
    char   filename[1024];

    hb_get_tempory_filename( h, filename, "%d_%d_%d",
                             hb_get_instance_id(h), title, preview );

    file = hb_fopen(filename, "wb");
    if( !file )
    {
        hb_error( "hb_save_preview: fopen failed (%s)", filename );
        return -1;
    }

    int pp, hh;
    for( pp = 0; pp < 3; pp++ )
    {
        uint8_t *data = buf->plane[pp].data;
        int stride = buf->plane[pp].stride;
        int w = buf->plane[pp].width;
        int h = buf->plane[pp].height;

        for( hh = 0; hh < h; hh++ )
        {
            fwrite( data, w, 1, file );
            data += stride;
        }
    }
    fclose( file );
    return 0;
}

hb_buffer_t * hb_read_preview( hb_handle_t * h, int title_idx, int preview )
{
    FILE * file;
    char   filename[1024];
    hb_title_set_t *title_set;

    hb_title_t * title = NULL;

    title_set = hb_get_title_set(h);

    int ii;
    for (ii = 0; ii < hb_list_count(title_set->list_title); ii++)
    {
        title = hb_list_item( title_set->list_title, ii);
        if (title != NULL && title->index == title_idx)
        {
            break;
        }
        title = NULL;
    }
    if (title == NULL)
    {
        hb_error( "hb_read_preview: invalid title (%d)", title_idx );
        return NULL;
    }

    hb_get_tempory_filename( h, filename, "%d_%d_%d",
                             hb_get_instance_id(h), title_idx, preview );

    file = hb_fopen(filename, "rb");
    if( !file )
    {
        hb_error( "hb_read_preview: fopen failed (%s)", filename );
        return NULL;
    }

    hb_buffer_t * buf;
    buf = hb_frame_buffer_init( AV_PIX_FMT_YUV420P, title->width, title->height );

    int pp, hh;
    for( pp = 0; pp < 3; pp++ )
    {
        uint8_t *data = buf->plane[pp].data;
        int stride = buf->plane[pp].stride;
        int w = buf->plane[pp].width;
        int h = buf->plane[pp].height;

        for( hh = 0; hh < h; hh++ )
        {
            fread( data, w, 1, file );
            data += stride;
        }
    }
    fclose( file );

    return buf;
}

/**
 * Create preview image of desired title a index of picture.
 * @param h Handle to hb_handle_t.
 * @param title Handle to hb_title_t of desired title.
 * @param picture Index in title.
 * @param buffer Handle to buffer were image will be drawn.
 */
void hb_get_preview( hb_handle_t * h, hb_job_t * job, int picture,
                     uint8_t * buffer )
{
    hb_title_t         * title = job->title;
    char                 filename[1024];
    hb_buffer_t        * in_buf, * deint_buf = NULL, * preview_buf;
    uint8_t            * pen;
    uint32_t             swsflags;
    AVPicture            pic_in, pic_preview, pic_deint, pic_crop;
    struct SwsContext  * context;
    int                  i;
    int                  preview_size;

    swsflags = SWS_LANCZOS | SWS_ACCURATE_RND;

    preview_buf = hb_frame_buffer_init( AV_PIX_FMT_RGB32,
                                        job->width, job->height );
    hb_avpicture_fill( &pic_preview, preview_buf );

    // Allocate the AVPicture frames and fill in

    memset( filename, 0, 1024 );

    in_buf = hb_read_preview( h, title->index, picture );
    if ( in_buf == NULL )
    {
        return;
    }

    hb_avpicture_fill( &pic_in, in_buf );

    if( job->deinterlace )
    {
        // Deinterlace and crop
        deint_buf = hb_frame_buffer_init( AV_PIX_FMT_YUV420P,
                                          title->width, title->height );
        hb_deinterlace(deint_buf, in_buf);
        hb_avpicture_fill( &pic_deint, deint_buf );

        av_picture_crop( &pic_crop, &pic_deint, AV_PIX_FMT_YUV420P,
                job->crop[0], job->crop[2] );
    }
    else
    {
        // Crop
        av_picture_crop( &pic_crop, &pic_in, AV_PIX_FMT_YUV420P, job->crop[0], job->crop[2] );
    }

    // Get scaling context
    context = hb_sws_get_context(title->width  - (job->crop[2] + job->crop[3]),
                             title->height - (job->crop[0] + job->crop[1]),
                             AV_PIX_FMT_YUV420P,
                             job->width, job->height, AV_PIX_FMT_RGB32,
                             swsflags);

    // Scale
    sws_scale(context,
              (const uint8_t* const *)pic_crop.data, pic_crop.linesize,
              0, title->height - (job->crop[0] + job->crop[1]),
              pic_preview.data, pic_preview.linesize);

    // Free context
    sws_freeContext( context );

    preview_size = pic_preview.linesize[0];
    pen = buffer;
    for( i = 0; i < job->height; i++ )
    {
        memcpy( pen, pic_preview.data[0] + preview_size * i, 4 * job->width );
        pen += 4 * job->width;
    }

    // Clean up
    hb_buffer_close( &in_buf );
    hb_buffer_close( &deint_buf );
    hb_buffer_close( &preview_buf );
}

 /**
 * Analyzes a frame to detect interlacing artifacts
 * and returns true if interlacing (combing) is found.
 *
 * Code taken from Thomas Oestreich's 32detect filter
 * in the Transcode project, with minor formatting changes.
 *
 * @param buf         An hb_buffer structure holding valid frame data
 * @param width       The frame's width in pixels
 * @param height      The frame's height in pixels
 * @param color_equal Sensitivity for detecting similar colors
 * @param color_diff  Sensitivity for detecting different colors
 * @param threshold   Sensitivity for flagging planes as combed
 * @param prog_equal  Sensitivity for detecting similar colors on progressive frames
 * @param prog_diff   Sensitivity for detecting different colors on progressive frames
 * @param prog_threshold Sensitivity for flagging progressive frames as combed
 */
int hb_detect_comb( hb_buffer_t * buf, int color_equal, int color_diff, int threshold, int prog_equal, int prog_diff, int prog_threshold )
{
    int j, k, n, off, cc_1, cc_2, cc[3];
	// int flag[3] ; // debugging flag
    uint16_t s1, s2, s3, s4;
    cc_1 = 0; cc_2 = 0;

    if ( buf->s.flags & 16 )
    {
        /* Frame is progressive, be more discerning. */
        color_diff = prog_diff;
        color_equal = prog_equal;
        threshold = prog_threshold;
    }

    /* One pas for Y, one pass for Cb, one pass for Cr */    
    for( k = 0; k < 3; k++ )
    {
        uint8_t * data = buf->plane[k].data;
        int width = buf->plane[k].width;
        int stride = buf->plane[k].stride;
        int height = buf->plane[k].height;

        for( j = 0; j < width; ++j )
        {
            off = 0;

            for( n = 0; n < ( height - 4 ); n = n + 2 )
            {
                /* Look at groups of 4 sequential horizontal lines */
                s1 = ( ( data )[ off + j              ] & 0xff );
                s2 = ( ( data )[ off + j +     stride ] & 0xff );
                s3 = ( ( data )[ off + j + 2 * stride ] & 0xff );
                s4 = ( ( data )[ off + j + 3 * stride ] & 0xff );

                /* Note if the 1st and 2nd lines are more different in
                   color than the 1st and 3rd lines are similar in color.*/
                if ( ( abs( s1 - s3 ) < color_equal ) &&
                     ( abs( s1 - s2 ) > color_diff ) )
                        ++cc_1;

                /* Note if the 2nd and 3rd lines are more different in
                   color than the 2nd and 4th lines are similar in color.*/
                if ( ( abs( s2 - s4 ) < color_equal ) &&
                     ( abs( s2 - s3 ) > color_diff) )
                        ++cc_2;

                /* Now move down 2 horizontal lines before starting over.*/
                off += 2 * stride;
            }
        }

        // compare results
        /*  The final cc score for a plane is the percentage of combed pixels it contains.
            Because sensitivity goes down to hundreths of a percent, multiply by 1000
            so it will be easy to compare against the threhold value which is an integer. */
        cc[k] = (int)( ( cc_1 + cc_2 ) * 1000.0 / ( width * height ) );
    }


    /* HandBrake is all yuv420, so weight the average percentage of all 3 planes accordingly.*/
    int average_cc = ( 2 * cc[0] + ( cc[1] / 2 ) + ( cc[2] / 2 ) ) / 3;
    
    /* Now see if that average percentage of combed pixels surpasses the threshold percentage given by the user.*/
    if( average_cc > threshold )
    {
#if 0
            hb_log("Average %i combed (Threshold %i) %i/%i/%i | PTS: %"PRId64" (%fs) %s", average_cc, threshold, cc[0], cc[1], cc[2], buf->start, (float)buf->start / 90000, (buf->flags & 16) ? "Film" : "Video" );
#endif
        return 1;
    }

#if 0
    hb_log("SKIPPED Average %i combed (Threshold %i) %i/%i/%i | PTS: %"PRId64" (%fs) %s", average_cc, threshold, cc[0], cc[1], cc[2], buf->start, (float)buf->start / 90000, (buf->flags & 16) ? "Film" : "Video" );
#endif

    /* Reaching this point means no combing detected. */
    return 0;

}

/**
 * Calculates job width and height for anamorphic content,
 *
 * @param job Handle to hb_job_t
 * @param output_width Pointer to returned storage width
 * @param output_height Pointer to returned storage height
 * @param output_par_width Pointer to returned pixel width
 * @param output_par_height Pointer to returned pixel height
 */
void hb_set_anamorphic_size( hb_job_t * job,
        int *output_width, int *output_height,
        int *output_par_width, int *output_par_height )
{
    /* Set up some variables to make the math easier to follow. */
    hb_title_t * title = job->title;
    int cropped_width = title->width - job->crop[2] - job->crop[3] ;
    int cropped_height = title->height - job->crop[0] - job->crop[1] ;
    double storage_aspect = (double)cropped_width / (double)cropped_height;
    int mod = job->modulus ? job->modulus : 16;
    double aspect = title->aspect;
    
    int64_t pixel_aspect_width  = job->anamorphic.par_width;
    int64_t pixel_aspect_height = job->anamorphic.par_height;

    /* If a source was really NTSC or PAL and the user specified ITU PAR
       values, replace the standard PAR values with the ITU broadcast ones. */
    if( title->width == 720 && job->anamorphic.itu_par )
    {
        // convert aspect to a scaled integer so we can test for 16:9 & 4:3
        // aspect ratios ignoring insignificant differences in the LSBs of
        // the floating point representation.
        int iaspect = aspect * 9.;

        /* Handle ITU PARs */
        if (title->height == 480)
        {
            /* It's NTSC */
            if (iaspect == 16)
            {
                /* It's widescreen */
                pixel_aspect_width = 40;
                pixel_aspect_height = 33;
            }
            else if (iaspect == 12)
            {
                /* It's 4:3 */
                pixel_aspect_width = 10;
                pixel_aspect_height = 11;
            }
        }
        else if (title->height == 576)
        {
            /* It's PAL */
            if(iaspect == 16)
            {
                /* It's widescreen */
                pixel_aspect_width = 16;
                pixel_aspect_height = 11;
            }
            else if (iaspect == 12)
            {
                /* It's 4:3 */
                pixel_aspect_width = 12;
                pixel_aspect_height = 11;
            }
        }
    }

    /* Figure out what width the source would display at. */
    int source_display_width = cropped_width * (double)pixel_aspect_width /
                               (double)pixel_aspect_height ;

    /*
       3 different ways of deciding output dimensions:
        - 1: Strict anamorphic, preserve source dimensions
        - 2: Loose anamorphic, round to mod16 and preserve storage aspect ratio
        - 3: Power user anamorphic, specify everything
    */
    int width, height;
    int maxWidth, maxHeight;

    maxWidth = MULTIPLE_MOD_DOWN( job->maxWidth, mod );
    maxHeight = MULTIPLE_MOD_DOWN( job->maxHeight, mod );

    switch( job->anamorphic.mode )
    {
        case 1:
            /* Strict anamorphic */
            *output_width  = MULTIPLE_MOD( cropped_width, 2 );
            *output_height = MULTIPLE_MOD( cropped_height, 2 );
            // adjust the source PAR for new width/height
            // new PAR = source PAR * ( old width / new_width ) * ( new_height / old_height )
            pixel_aspect_width = (int64_t)title->pixel_aspect_width * cropped_width * (*output_height);            
            pixel_aspect_height = (int64_t)title->pixel_aspect_height * (*output_width) * cropped_height;
        break;

        case 2:
            /* "Loose" anamorphic.
                - Uses mod16-compliant dimensions,
                - Allows users to set the width
            */
            width = job->width;
            // height: Gets set later, ignore user job->height value

            /* Gotta handle bounding dimensions.
               If the width is too big, just reset it with no rescaling.
               Instead of using the aspect-scaled job height,
               we need to see if the job width divided by the storage aspect
               is bigger than the max. If so, set it to the max (this is sloppy).
               If not, set job height to job width divided by storage aspect.
            */

            /* Time to get picture width that divide cleanly.*/
            width  = MULTIPLE_MOD( width, mod);

            if ( maxWidth && (maxWidth < job->width) )
                width = maxWidth;

            /* Verify these new dimensions don't violate max height and width settings */
            height = ((double)width / storage_aspect) + 0.5;

            /* Time to get picture height that divide cleanly.*/
            height = MULTIPLE_MOD( height, mod);
            
            if ( maxHeight && (maxHeight < height) )
            {
                height = maxHeight;
                width = ((double)height * storage_aspect) + 0.5;
                width  = MULTIPLE_MOD( width, mod);
            }

            /* The film AR is the source's display width / cropped source height.
               The output display width is the output height * film AR.
               The output PAR is the output display width / output storage width. */
            pixel_aspect_width = (int64_t)height * cropped_width * pixel_aspect_width;
            pixel_aspect_height = (int64_t)width * cropped_height * pixel_aspect_height;

            /* Pass the results back to the caller */
            *output_width = width;
            *output_height = height;
        break;
            
        case 3:
            /* Anamorphic 3: Power User Jamboree
               - Set everything based on specified values */
            
            /* Use specified storage dimensions */
            storage_aspect = (double)job->width / (double)job->height;
            width = job->width;
            height = job->height;
            
            /* Time to get picture dimensions that divide cleanly.*/
            width  = MULTIPLE_MOD( width, mod);
            height = MULTIPLE_MOD( height, mod);
            
            /* Bind to max dimensions */
            if( maxWidth && width > maxWidth )
            {
                width = maxWidth;
                // If we are keeping the display aspect, then we are going
                // to be modifying the PAR anyway.  So it's preferred
                // to let the width/height stray some from the original
                // requested storage aspect.
                //
                // But otherwise, PAR and DAR will change the least
                // if we stay as close as possible to the requested
                // storage aspect.
                if ( !job->anamorphic.keep_display_aspect )
                {
                    height = ((double)width / storage_aspect) + 0.5;
                    height = MULTIPLE_MOD( height, mod);
                }
            }
            if( maxHeight && height > maxHeight )
            {
                height = maxHeight;
                // Ditto, see comment above
                if ( !job->anamorphic.keep_display_aspect )
                {
                    width = ((double)height * storage_aspect) + 0.5;
                    width  = MULTIPLE_MOD( width, mod);
                }
            }
            
            /* That finishes the storage dimensions. On to display. */            
            if( job->anamorphic.dar_width && job->anamorphic.dar_height )
            {
                /* We need to adjust the PAR to produce this aspect. */
                pixel_aspect_width = (int64_t)height * job->anamorphic.dar_width / job->anamorphic.dar_height;
                pixel_aspect_height = width;
            }
            else
            {
                /* If we're doing ana 3 and not specifying a DAR, care needs to be taken.
                   This indicates a PAR is potentially being set by the interface. But
                   this is an output PAR, to correct a source, and it should not be assumed
                   that it properly creates a display aspect ratio when applied to the source,
                   which could easily be stored in a different resolution. */
                if( job->anamorphic.keep_display_aspect )
                {
                    /* We can ignore the possibility of a PAR change */
                    pixel_aspect_width = (int64_t)height * ( (double)source_display_width / (double)cropped_height );
                    pixel_aspect_height = width;
                }
                else
                {
                    int output_display_width = width * (double)pixel_aspect_width /
                        (double)pixel_aspect_height;
                    pixel_aspect_width = output_display_width;
                    pixel_aspect_height = width;
                }
            }
            
            /* Back to caller */
            *output_width = width;
            *output_height = height;
        break;
    }
    
    /* While x264 is smart enough to reduce fractions on its own, libavcodec
     * needs some help with the math, so lose superfluous factors. */
    hb_limit_rational64( &pixel_aspect_width, &pixel_aspect_height,
                        pixel_aspect_width, pixel_aspect_height, 65535 );
    hb_reduce( output_par_width, output_par_height,
               pixel_aspect_width, pixel_aspect_height );
}

/**
 * Add a filter to a jobs filter list
 *
 * @param job Handle to hb_job_t
 * @param settings to give the filter
 */
void hb_add_filter( hb_job_t * job, hb_filter_object_t * filter, const char * settings_in )
{
    char * settings = NULL;

    if ( settings_in != NULL )
    {
        settings = strdup( settings_in );
    }
    filter->settings = settings;
    if( filter->enforce_order )
    {
        // Find the position in the filter chain this filter belongs in
        int i;
        for( i = 0; i < hb_list_count( job->list_filter ); i++ )
        {
            hb_filter_object_t * f = hb_list_item( job->list_filter, i );
            if( f->id > filter->id )
            {
                hb_list_insert( job->list_filter, i, filter );
                return;
            }
            else if( f->id == filter->id )
            {
                // Don't allow the same filter to be added twice
                hb_filter_close( &filter );
                return;
            }
        }
    }
    // No position found or order not enforced for this filter
    hb_list_add( job->list_filter, filter );
}

/**
 * Validate and adjust dimensions if necessary
 *
 * @param job Handle to hb_job_t
 */
void hb_validate_size( hb_job_t * job )
{
    if ( job->anamorphic.mode )
    {
        hb_set_anamorphic_size( job, &job->width, &job->height,
            &job->anamorphic.par_width, &job->anamorphic.par_height );
    }
    else
    {
        if ( job->maxHeight && ( job->height > job->maxHeight )  )
        {
            job->height = job->maxHeight;
            hb_fix_aspect( job, HB_KEEP_HEIGHT );
            hb_log( "Height out of bounds, scaling down to %i",
                    job->maxHeight );
            hb_log( "New dimensions %i * %i", job->width, job->height );
        }
        if ( job->maxWidth && ( job->width > job->maxWidth )  )
        {
            job->width = job->maxWidth;
            hb_fix_aspect( job, HB_KEEP_WIDTH );
            hb_log( "Width out of bounds, scaling down to %i",
                    job->maxWidth );
            hb_log( "New dimensions %i * %i", job->width, job->height );
        }
    }
}

/**
 * Calculates job width, height, and cropping parameters.
 * @param job Handle to hb_job_t.
 * @param aspect Desired aspect ratio. Value of -1 uses title aspect.
 * @param pixels Maximum desired pixel count.
 */
void hb_set_size( hb_job_t * job, double aspect, int pixels )
{
    hb_title_t * title = job->title;

    int croppedWidth  = title->width - title->crop[2] - title->crop[3];
    int croppedHeight = title->height - title->crop[0] - title->crop[1];
    double croppedAspect = title->aspect * title->height * croppedWidth /
                           croppedHeight / title->width;
    int addCrop;
    int i, w, h;

    if( aspect <= 0 )
    {
        /* Keep the best possible aspect ratio */
        aspect = croppedAspect;
    }

    /* Crop if necessary to obtain the desired ratio */
    memcpy( job->crop, title->crop, 4 * sizeof( int ) );
    if( aspect < croppedAspect )
    {
        /* Need to crop on the left and right */
        addCrop = croppedWidth - aspect * croppedHeight * title->width /
                    title->aspect / title->height;
        if( addCrop & 3 )
        {
            addCrop = ( addCrop + 1 ) / 2;
            job->crop[2] += addCrop;
            job->crop[3] += addCrop;
        }
        else if( addCrop & 2 )
        {
            addCrop /= 2;
            job->crop[2] += addCrop - 1;
            job->crop[3] += addCrop + 1;
        }
        else
        {
            addCrop /= 2;
            job->crop[2] += addCrop;
            job->crop[3] += addCrop;
        }
    }
    else if( aspect > croppedAspect )
    {
        /* Need to crop on the top and bottom */
        addCrop = croppedHeight - croppedWidth * title->aspect *
            title->height / aspect / title->width;
        if( addCrop & 3 )
        {
            addCrop = ( addCrop + 1 ) / 2;
            job->crop[0] += addCrop;
            job->crop[1] += addCrop;
        }
        else if( addCrop & 2 )
        {
            addCrop /= 2;
            job->crop[0] += addCrop - 1;
            job->crop[1] += addCrop + 1;
        }
        else
        {
            addCrop /= 2;
            job->crop[0] += addCrop;
            job->crop[1] += addCrop;
        }
    }

    /* Compute a resolution from the number of pixels and aspect */
    for( i = 0;; i++ )
    {
        w = 16 * i;
        h = MULTIPLE_16( (int)( (double)w / aspect ) );
        if( w * h > pixels )
        {
            break;
        }
    }
    i--;
    job->width  = 16 * i;
    job->height = MULTIPLE_16( (int)( (double)job->width / aspect ) );
}

/**
 * Returns the number of jobs in the queue.
 * @param h Handle to hb_handle_t.
 * @return Number of jobs.
 */
int hb_count( hb_handle_t * h )
{
    return hb_list_count( h->jobs );
}

/**
 * Returns handle to job at index i within the job list.
 * @param h Handle to hb_handle_t.
 * @param i Index of job.
 * @returns Handle to hb_job_t of desired job.
 */
hb_job_t * hb_job( hb_handle_t * h, int i )
{
    return hb_list_item( h->jobs, i );
}

hb_job_t * hb_current_job( hb_handle_t * h )
{
    return( h->current_job );
}

/**
 * Adds a job to the job list.
 * @param h Handle to hb_handle_t.
 * @param job Handle to hb_job_t.
 */
void hb_add( hb_handle_t * h, hb_job_t * job )
{
    hb_job_t      * job_copy;
    hb_audio_t    * audio;
    hb_subtitle_t * subtitle;
    int             i;
    char            audio_lang[4];

    /* Copy the job */
    job_copy        = calloc( sizeof( hb_job_t ), 1 );
    memcpy( job_copy, job, sizeof( hb_job_t ) );

    /* If we're doing Foreign Audio Search, copy all subtitles matching the
     * first audio track language we find in the audio list.
     *
     * Otherwise, copy all subtitles found in the input job (which can be
     * manually selected by the user, or added after the Foreign Audio
     * Search pass). */
    memset( audio_lang, 0, sizeof( audio_lang ) );

    if( job->indepth_scan )
    {

        /* Find the first audio language that is being encoded, then add all the
         * matching subtitles for that language. */
        for( i = 0; i < hb_list_count( job->list_audio ); i++ )
        {
            if( ( audio = hb_list_item( job->list_audio, i ) ) )
            {
                strncpy( audio_lang, audio->config.lang.iso639_2, sizeof( audio_lang ) );
                break;
            }
        }

        /*
         * If doing a subtitle scan then add all the matching subtitles for this
         * language.
         */
        job_copy->list_subtitle = hb_list_init();
    
        for( i = 0; i < hb_list_count( job->title->list_subtitle ); i++ )
        {
            subtitle = hb_list_item( job->title->list_subtitle, i );
            if( strcmp( subtitle->iso639_2, audio_lang ) == 0 &&
                hb_subtitle_can_force( subtitle->source ) )
            {
                /* Matched subtitle language with audio language, so add this to
                 * our list to scan.
                 *
                 * We will update the subtitle list on the next pass later, after
                 * the subtitle scan pass has completed. */
                hb_list_add( job_copy->list_subtitle,
                             hb_subtitle_copy( subtitle ) );
            }
        }
    }
    else
    {
        /* Copy all subtitles from the input job to title_copy/job_copy. */
        job_copy->list_subtitle = hb_subtitle_list_copy( job->list_subtitle );
    }

    job_copy->list_chapter = hb_chapter_list_copy( job->list_chapter );
    job_copy->list_audio = hb_audio_list_copy( job->list_audio );
    job_copy->list_attachment = hb_attachment_list_copy( job->list_attachment );
    job_copy->metadata = hb_metadata_copy( job->metadata );

    if ( job->file )
        job_copy->file  = strdup( job->file );
    if ( job->advanced_opts )
        job_copy->advanced_opts  = strdup( job->advanced_opts );
    if ( job->x264_preset )
        job_copy->x264_preset  = strdup( job->x264_preset );
    if ( job->x264_tune )
        job_copy->x264_tune  = strdup( job->x264_tune );
    if ( job->h264_profile )
        job_copy->h264_profile  = strdup( job->h264_profile );
    if ( job->h264_level )
        job_copy->h264_level  = strdup( job->h264_level );

    job_copy->h     = h;
    job_copy->pause = h->pause_lock;

    /* Copy the job filter list */
    job_copy->list_filter = hb_filter_list_copy( job->list_filter );

    /* Add the job to the list */
    hb_list_add( h->jobs, job_copy );
    h->job_count = hb_count(h);
    h->job_count_permanent++;
}

/**
 * Removes a job from the job list.
 * @param h Handle to hb_handle_t.
 * @param job Handle to hb_job_t.
 */
void hb_rem( hb_handle_t * h, hb_job_t * job )
{
    hb_list_rem( h->jobs, job );

    h->job_count = hb_count(h);
    if (h->job_count_permanent)
        h->job_count_permanent--;

    /* XXX free everything XXX */
}

/**
 * Starts the conversion process.
 * Sets state to HB_STATE_WORKING.
 * calls hb_work_init, to launch work thread. Stores handle to work thread.
 * @param h Handle to hb_handle_t.
 */
void hb_start( hb_handle_t * h )
{
    /* XXX Hack */
    h->job_count = hb_list_count( h->jobs );
    h->job_count_permanent = h->job_count;

    hb_lock( h->state_lock );
    h->state.state = HB_STATE_WORKING;
#define p h->state.param.working
    p.progress  = 0.0;
    p.job_cur   = 1;
    p.job_count = h->job_count;
    p.rate_cur  = 0.0;
    p.rate_avg  = 0.0;
    p.hours     = -1;
    p.minutes   = -1;
    p.seconds   = -1;
    p.sequence_id = 0;
#undef p
    hb_unlock( h->state_lock );

    h->paused = 0;

    h->work_die    = 0;
    h->work_error  = HB_ERROR_NONE;
    h->work_thread = hb_work_init( h->jobs, &h->work_die, &h->work_error, &h->current_job );
}

/**
 * Pauses the conversion process.
 * @param h Handle to hb_handle_t.
 */
void hb_pause( hb_handle_t * h )
{
    if( !h->paused )
    {
        hb_lock( h->pause_lock );
        h->paused = 1;

        hb_current_job( h )->st_pause_date = hb_get_date();

        hb_lock( h->state_lock );
        h->state.state = HB_STATE_PAUSED;
        hb_unlock( h->state_lock );
    }
}

/**
 * Resumes the conversion process.
 * @param h Handle to hb_handle_t.
 */
void hb_resume( hb_handle_t * h )
{
    if( h->paused )
    {
#define job hb_current_job( h )
        if( job->st_pause_date != -1 )
        {
           job->st_paused += hb_get_date() - job->st_pause_date;
        }
#undef job

        hb_unlock( h->pause_lock );
        h->paused = 0;
    }
}

/**
 * Stops the conversion process.
 * @param h Handle to hb_handle_t.
 */
void hb_stop( hb_handle_t * h )
{
    h->work_die = 1;

    h->job_count = hb_count(h);
    h->job_count_permanent = 0;

    hb_resume( h );
}

/**
 * Stops the conversion process.
 * @param h Handle to hb_handle_t.
 */
void hb_scan_stop( hb_handle_t * h )
{
    h->scan_die = 1;

    h->job_count = hb_count(h);
    h->job_count_permanent = 0;

    hb_resume( h );
}

/**
 * Returns the state of the conversion process.
 * @param h Handle to hb_handle_t.
 * @param s Handle to hb_state_t which to copy the state data.
 */
void hb_get_state( hb_handle_t * h, hb_state_t * s )
{
    hb_lock( h->state_lock );

    memcpy( s, &h->state, sizeof( hb_state_t ) );
    if ( h->state.state == HB_STATE_SCANDONE || h->state.state == HB_STATE_WORKDONE )
        h->state.state = HB_STATE_IDLE;

    hb_unlock( h->state_lock );
}

void hb_get_state2( hb_handle_t * h, hb_state_t * s )
{
    hb_lock( h->state_lock );

    memcpy( s, &h->state, sizeof( hb_state_t ) );

    hb_unlock( h->state_lock );
}

/**
 * Called in MacGui in UpdateUI to check
 *  for a new scan being completed to set a new source
 */
int hb_get_scancount( hb_handle_t * h)
 {
     return h->scanCount;
 }

/**
 * Closes access to libhb by freeing the hb_handle_t handle ontained in hb_init.
 * @param _h Pointer to handle to hb_handle_t.
 */
void hb_close( hb_handle_t ** _h )
{
    hb_handle_t * h = *_h;
    hb_title_t * title;

    h->die = 1;
    
    hb_thread_close( &h->main_thread );

    while( ( title = hb_list_item( h->title_set.list_title, 0 ) ) )
    {
        hb_list_rem( h->title_set.list_title, title );
        hb_title_close( &title );
    }
    hb_list_close( &h->title_set.list_title );

    hb_list_close( &h->jobs );
    hb_lock_close( &h->state_lock );
    hb_lock_close( &h->pause_lock );

    hb_system_sleep_opaque_close(&h->system_sleep_opaque);

    free( h->interjob );

    free( h );
    *_h = NULL;
}

int hb_global_init()
{
    int result = 0;

    result = hb_platform_init();
    if (result < 0)
    {
        hb_error("Platform specific initialization failed!");
        return -1;
    }

#ifdef USE_QSV
    result = hb_qsv_info_init();
    if (result < 0)
    {
        hb_error("hb_qsv_info_init failed!");
        return -1;
    }
#endif

    /* libavcodec */
    hb_avcodec_init();

    /* HB work objects */
    hb_register(&hb_muxer);
    hb_register(&hb_reader);
    hb_register(&hb_sync_video);
    hb_register(&hb_sync_audio);
    hb_register(&hb_deca52);
    hb_register(&hb_decavcodecv);
    hb_register(&hb_decavcodeca);
    hb_register(&hb_declpcm);
    hb_register(&hb_deccc608);
    hb_register(&hb_decpgssub);
    hb_register(&hb_decsrtsub);
    hb_register(&hb_decssasub);
    hb_register(&hb_dectx3gsub);
    hb_register(&hb_decutf8sub);
    hb_register(&hb_decvobsub);
    hb_register(&hb_encvobsub);
    hb_register(&hb_encavcodec);
    hb_register(&hb_encavcodeca);
#ifdef __APPLE__
    hb_register(&hb_encca_aac);
    hb_register(&hb_encca_haac);
#endif
#ifdef USE_FAAC
    hb_register(&hb_encfaac);
#endif
    hb_register(&hb_enclame);
    hb_register(&hb_enctheora);
    hb_register(&hb_encvorbis);
    hb_register(&hb_encx264);
#ifdef USE_QSV
    hb_register(&hb_encqsv);
#endif
    
    hb_common_global_init();

    return result;
}

/**
 * Cleans up libhb at a process level. Call before the app closes. Removes preview directory.
 */
void hb_global_close()
{
    char dirname[1024];
    DIR * dir;
    struct dirent * entry;
    
    /* Find and remove temp folder */
    memset( dirname, 0, 1024 );
    hb_get_temporary_directory( dirname );

    dir = opendir( dirname );
    if (dir)
    {
        while( ( entry = readdir( dir ) ) )
        {
            char filename[1024];
            if( entry->d_name[0] == '.' )
            {
                continue;
            }
            memset( filename, 0, 1024 );
            snprintf( filename, 1023, "%s/%s", dirname, entry->d_name );
            unlink( filename );
        }
        closedir( dir );
        rmdir( dirname );
    }
}

/**
 * Monitors the state of the update, scan, and work threads.
 * Sets scan done state when scan thread exits.
 * Sets work done state when work thread exits.
 * @param _h Handle to hb_handle_t
 */
static void thread_func( void * _h )
{
    hb_handle_t * h = (hb_handle_t *) _h;
    char dirname[1024];

    h->pid = getpid();

    /* Create folder for temporary files */
    memset( dirname, 0, 1024 );
    hb_get_temporary_directory( dirname );

    hb_mkdir( dirname );

    while( !h->die )
    {
        /* In case the check_update thread hangs, it'll die sooner or
           later. Then, we join it here */
        if( h->update_thread &&
            hb_thread_has_exited( h->update_thread ) )
        {
            hb_thread_close( &h->update_thread );
        }

        /* Check if the scan thread is done */
        if( h->scan_thread &&
            hb_thread_has_exited( h->scan_thread ) )
        {
            hb_thread_close( &h->scan_thread );

            if ( h->scan_die )
            {
                hb_title_t * title;

                hb_remove_previews( h );
                while( ( title = hb_list_item( h->title_set.list_title, 0 ) ) )
                {
                    hb_list_rem( h->title_set.list_title, title );
                    hb_title_close( &title );
                }

                hb_log( "hb_scan: canceled" );
            }
            else
            {
                hb_log( "libhb: scan thread found %d valid title(s)",
                        hb_list_count( h->title_set.list_title ) );
            }
            hb_lock( h->state_lock );
            h->state.state = HB_STATE_SCANDONE; //originally state.state
			hb_unlock( h->state_lock );
			/*we increment this sessions scan count by one for the MacGui
			to trigger a new source being set */
            h->scanCount++;
        }

        /* Check if the work thread is done */
        if( h->work_thread &&
            hb_thread_has_exited( h->work_thread ) )
        {
            hb_thread_close( &h->work_thread );

            hb_log( "libhb: work result = %d",
                    h->work_error );
            hb_lock( h->state_lock );
            h->state.state                = HB_STATE_WORKDONE;
            h->state.param.workdone.error = h->work_error;

            h->job_count = hb_count(h);
            if (h->job_count < 1)
                h->job_count_permanent = 0;
            hb_unlock( h->state_lock );
        }

        hb_snooze( 50 );
    }

    if( h->scan_thread )
    {
        hb_scan_stop( h );
        hb_thread_close( &h->scan_thread );
    }
    if( h->work_thread )
    {
        hb_stop( h );
        hb_thread_close( &h->work_thread );
    }
    hb_remove_previews( h );
}

/**
 * Redirects stderr to the registered callback
 * function.
 * @param _data Unused.
 */
static void redirect_thread_func(void * _data)
{
    int pfd[2];
    pipe(pfd);
#if defined( SYS_MINGW )
    // dup2 doesn't work on windows for some stupid reason
    stderr->_file = pfd[1];
#else
    dup2(pfd[1], /*stderr*/ 2);
#endif
    FILE * log_f = fdopen(pfd[0], "rb");
    
    char line_buffer[500];
    while(fgets(line_buffer, 500, log_f) != NULL)
    {
        hb_log_callback(line_buffer);
    }
}

/**
 * Returns the PID.
 * @param h Handle to hb_handle_t
 */
int hb_get_pid( hb_handle_t * h )
{
    return h->pid;
}

/**
 * Returns the id for the given instance.
 * @param h Handle to hb_handle_t
 * @returns The ID for the given instance
 */
int hb_get_instance_id( hb_handle_t * h )
{
    return h->id;
}

/**
 * Sets the current state.
 * @param h Handle to hb_handle_t
 * @param s Handle to new hb_state_t
 */
void hb_set_state( hb_handle_t * h, hb_state_t * s )
{
    hb_lock( h->pause_lock );
    hb_lock( h->state_lock );
    memcpy( &h->state, s, sizeof( hb_state_t ) );
    if( h->state.state == HB_STATE_WORKING ||
        h->state.state == HB_STATE_SEARCHING )
    {
        /* XXX Hack */
        if (h->job_count < 1)
            h->job_count_permanent = 1;

        h->state.param.working.job_cur =
            h->job_count_permanent - hb_list_count( h->jobs );
        h->state.param.working.job_count = h->job_count_permanent;

        // Set which job is being worked on
        if (h->current_job)
            h->state.param.working.sequence_id = h->current_job->sequence_id;
        else
            h->state.param.working.sequence_id = 0;
    }
    hb_unlock( h->state_lock );
    hb_unlock( h->pause_lock );
}

void hb_system_sleep_allow(hb_handle_t *h)
{
    hb_system_sleep_private_enable(h->system_sleep_opaque);
}

void hb_system_sleep_prevent(hb_handle_t *h)
{
    hb_system_sleep_private_disable(h->system_sleep_opaque);
}

/* Passes a pointer to persistent data */
hb_interjob_t * hb_interjob_get( hb_handle_t * h )
{
    return h->interjob;
}
