/* hb.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/handbrake.h"
#include "handbrake/hbffmpeg.h"
#include "handbrake/hbavfilter.h"
#include "handbrake/encx264.h"
#include "libavfilter/avfilter.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <turbojpeg.h>

#if HB_PROJECT_FEATURE_QSV
#include "handbrake/qsv_common.h"
#endif

#if defined( SYS_MINGW )
#include <io.h>
#if defined(PTW32_VERSION)
#include <pthread.h>
#endif
#endif

struct hb_handle_s
{
    int            id;

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
    int            sequence_id;
    hb_list_t    * jobs;
    hb_job_t     * current_job;
    volatile int   work_die;
    hb_error_code  work_error;
    hb_thread_t  * work_thread;

    hb_lock_t    * state_lock;
    hb_state_t     state;

    int            paused;
    hb_lock_t    * pause_lock;
    int64_t        pause_date;
    int64_t        pause_duration;

    volatile int   scan_die;

    /* Stash of persistent data between jobs, for stuff
       like correcting frame count and framerate estimates
       on multi-pass encodes where frames get dropped.     */
    hb_interjob_t * interjob;

    // power management opaque pointer
    void         * system_sleep_opaque;
};

hb_work_object_t * hb_objects = NULL;
int hb_instance_counter = 0;
int disable_hardware = 0;

static void thread_func( void * );

int hb_avcodec_open(AVCodecContext *avctx, const AVCodec *codec,
                    AVDictionary **av_opts, int thread_count)
{
    int ret;

    if ((thread_count == HB_FFMPEG_THREADS_AUTO || thread_count > 0) &&
        (codec->type == AVMEDIA_TYPE_VIDEO))
    {
        avctx->thread_count = (thread_count == HB_FFMPEG_THREADS_AUTO) ?
                               hb_get_cpu_count() / 2 + 1 : thread_count;
        avctx->thread_type = FF_THREAD_FRAME|FF_THREAD_SLICE;
    }
    else
    {
        avctx->thread_count = 1;
    }

    if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
    {
        // "experimental" encoders will not open without this
        avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    ret = avcodec_open2(avctx, codec, av_opts);
    return ret;
}

void hb_avcodec_free_context(AVCodecContext **avctx)
{
    avcodec_free_context(avctx);
}


int hb_picture_fill(uint8_t *data[], int stride[], hb_buffer_t *buf)
{
    int ret, ii;

    if (buf->f.max_plane < 0)
    {
        return -1;
    }

    for (ii = 0; ii <= buf->f.max_plane; ii++)
        stride[ii] = buf->plane[ii].stride;
    for (; ii < 4; ii++)
        stride[ii] = stride[ii - 1];

    ret = av_image_fill_pointers(data, buf->f.fmt,
                                 buf->plane[0].height,
                                 buf->data, stride);
    if (ret != buf->size)
    {
        hb_error("Internal error hb_picture_fill expected %d, got %d",
                 buf->size, ret);
    }
    return ret;
}

int hb_picture_crop(uint8_t *data[], int stride[], hb_buffer_t *buf,
                    int top, int left)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(buf->f.fmt);
    int x_shift, y_shift;

    if (desc == NULL)
        return -1;

    x_shift = desc->log2_chroma_w;
    y_shift = desc->log2_chroma_h;

    data[0] = buf->plane[0].data + top * buf->plane[0].stride + left;
    data[1] = buf->plane[1].data + (top >> y_shift) * buf->plane[1].stride +
              (left >> x_shift);
    data[2] = buf->plane[2].data + (top >> y_shift) * buf->plane[2].stride +
              (left >> x_shift);
    data[3] = NULL;

    stride[0] = buf->plane[0].stride;
    stride[1] = buf->plane[1].stride;
    stride[2] = buf->plane[2].stride;
    stride[3] = 0;

    return 0;
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

void hb_log_level_set(hb_handle_t *h, int level)
{
    global_verbosity_level = level;
}

/**
 * libhb initialization routine.
 * @param verbose HB_DEBUG_NONE or HB_DEBUG_ALL.
 * @return Handle to hb_handle_t for use on all subsequent calls to libhb.
 */
hb_handle_t * hb_init( int verbose )
{
    hb_handle_t * h = calloc( sizeof( hb_handle_t ), 1 );

    /* See hb_deep_log() and hb_log() in common.c */
    hb_log_level_set(h, verbose);

    h->id = hb_instance_counter++;

    /* Initialize opaque for PowerManagement purposes */
    h->system_sleep_opaque = hb_system_sleep_opaque_init();

	h->title_set.list_title = hb_list_init();
    h->jobs       = hb_list_init();

    h->state_lock  = hb_lock_init();
    h->state.state = HB_STATE_IDLE;

    h->pause_lock = hb_lock_init();
    h->pause_date = -1;

    h->interjob = calloc( sizeof( hb_interjob_t ), 1 );

    /* Start library thread */
    hb_log( "hb_init: starting libhb thread" );
    h->die         = 0;
    h->main_thread = hb_thread_init( "libhb", thread_func, h,
                                     HB_NORMAL_PRIORITY );

    return h;
}

// Make sure these strings at least exist in the executable even though
// they may not all be visible in the frontend.
static const char* hb_title          = HB_PROJECT_TITLE;
static const char* hb_name           = HB_PROJECT_NAME;
static const char* hb_website        = HB_PROJECT_URL_WEBSITE;
static const char* hb_community      = HB_PROJECT_URL_COMMUNITY;
static const char* hb_irc            = HB_PROJECT_URL_IRC;
static const char* hb_version        = HB_PROJECT_VERSION;
static const int   hb_build          = HB_PROJECT_BUILD;
static const char* hb_repo_url       = HB_PROJECT_REPO_URL;
static const char* hb_repo_tag       = HB_PROJECT_REPO_TAG;
static const int   hb_repo_rev       = HB_PROJECT_REPO_REV;
static const char* hb_repo_hash      = HB_PROJECT_REPO_HASH;
static const char* hb_repo_branch    = HB_PROJECT_REPO_BRANCH;
static const char* hb_repo_remote    = HB_PROJECT_REPO_REMOTE;
static const char* hb_repo_type      = HB_PROJECT_REPO_TYPE;

const char * hb_get_full_description()
{
    static char * desc = NULL;
    if (desc == NULL)
    {
        desc = hb_strdup_printf("%s\n"
                                "\tWebsite:     %s\n"
                                "\tForum:       %s\n"
                                "\tIRC:         %s\n"
                                "\tBuild Type:  %s\n"
                                "\tRepository:  %s\n"
                                "\tRelease Tag: %s\n"
                                "\tRevision:    %d\n"
                                "\tCommit Hash: %s\n"
                                "\tBranch:      %s\n"
                                "\tRemote:      %s",
                                hb_title, hb_website, hb_community, hb_irc,
                                hb_repo_type, hb_repo_url, hb_repo_tag, hb_repo_rev,
                                hb_repo_hash, hb_repo_branch, hb_repo_remote);
    }
    return desc;
}

/**
 * Returns current version of libhb.
 * @param h Handle to hb_handle_t.
 * @return character array of version number.
 */
const char * hb_get_version( hb_handle_t * h )
{
    // Silence compiler warnings for unused variables
    ((void)(hb_title));
    ((void)(hb_name));
    ((void)(hb_website));
    ((void)(hb_community));
    ((void)(hb_irc));
    ((void)(hb_version));
    ((void)(hb_repo_url));
    ((void)(hb_repo_tag));
    ((void)(hb_repo_rev));
    ((void)(hb_repo_hash));
    ((void)(hb_repo_branch));
    ((void)(hb_repo_remote));
    ((void)(hb_repo_type));
    return hb_version;
}

/**
 * Returns current build of libhb.
 * @param h Handle to hb_handle_t.
 * @return character array of build number.
 */
int hb_get_build( hb_handle_t * h )
{
    return hb_build;
}

/**
 * Deletes current previews associated with titles
 * @param h Handle to hb_handle_t
 */
void hb_remove_previews( hb_handle_t * h )
{
    char          * filename;
    const char    * dirname;
    hb_title_t    * title;
    int             i, count, len;
    DIR           * dir;
    struct dirent * entry;

    dirname = hb_get_temporary_directory();
    dir = opendir( dirname );
    if (dir == NULL)
    {
        return;
    }

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
            filename = hb_strdup_printf("%d_%d", h->id, title->index);
            len = snprintf( filename, 1024, "%d_%d", h->id, title->index );
            if (strncmp(entry->d_name, filename, len) == 0)
            {
                free(filename);
                filename = hb_strdup_printf("%s/%s", dirname, entry->d_name);
                int ulerr = unlink( filename );
                if (ulerr < 0) {
                    hb_log("Unable to remove preview: %i - %s", ulerr, filename);
                }
                free(filename);
                break;
            }
            free(filename);
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
 * @param min_duration Ignore titles below a given threshold
 * @param crop_threshold_frames The number of frames to trigger smart crop
 * @param crop_threshold_pixels The variance in pixels detected that are allowed for.
 * @param exclude_extensions A list of extensions to exclude for this scan.
 * @param hw_decode  The preferred hardware decoder to use..
 * @param keep_duplicate_titles
 */
void hb_scan( hb_handle_t * h, hb_list_t * paths, int title_index,
              int preview_count, int store_previews, uint64_t min_duration,
              int crop_threshold_frames, int crop_threshold_pixels,
              hb_list_t * exclude_extensions, int hw_decode, int keep_duplicate_titles)
{
    hb_title_t * title;

    char *single_path = NULL;
    int path_count = hb_list_count(paths);

    if (path_count == 1)
    {
        single_path = hb_list_item(paths, 0);
    }
    
    // Check if scanning is necessary. Only works on Single Path.
    if (single_path != NULL && h->title_set.path != NULL && !strcmp(h->title_set.path, single_path))
    {
        // Current title_set path matches requested single_path.
        // Check if the requested title has already been scanned.
        int ii;
        for (ii = 0; ii < hb_list_count(h->title_set.list_title); ii++)
        {
            title = hb_list_item(h->title_set.list_title, ii);
            if (title->index == title_index)
            {
                // In some cases, we don't care what the preview count is.
                // E.g. when rescanning at the start of a job. In these
                // cases, the caller can set preview_count to -1 to tell
                // us to use the same count as the previous scan, if known.
                if (preview_count < 0)
                {
                    preview_count = title->preview_count;
                }
                if (preview_count == title->preview_count)
                {
                    // Title has already been scanned.
                    hb_lock( h->state_lock );
                    h->state.state = HB_STATE_SCANDONE;
                    hb_unlock( h->state_lock );
                    return;
                }
            }
        }
    }
    if (preview_count < 0)
    {
        preview_count = 10;
    }

    h->scan_die = 0;

    /* Clean up from previous scan */
    hb_remove_previews( h );
    while( ( title = hb_list_item( h->title_set.list_title, 0 ) ) )
    {
        hb_list_rem( h->title_set.list_title, title );
        hb_title_close( &title );
    }
    free((char*)h->title_set.path);
    h->title_set.path = NULL;

    /* Print CPU info here so that it's in all scan and encode logs */
    const char *cpu_name = hb_get_cpu_name();
    const char *cpu_type = hb_get_cpu_platform_name();
    hb_log("CPU: %s", cpu_name != NULL ? cpu_name : "");
    if (cpu_type != NULL)
    {
        hb_log(" - %s", cpu_type);
    }
    hb_log(" - logical processor count: %d", hb_get_cpu_count());

#if HB_PROJECT_FEATURE_QSV
    if (!is_hardware_disabled())
    {
        /* Print QSV info here so that it's in all scan and encode logs */
        hb_qsv_info_print();
    }
#endif

    char *path_info = single_path;
    if (path_count > 1)
    {
        path_info = "(multiple)";
    }

    hb_log( "hb_scan: path=%s, title_index=%d", path_info, title_index );
    h->scan_thread = hb_scan_init( h, &h->scan_die, paths, title_index,
                                   &h->title_set, preview_count,
                                   store_previews, min_duration,
                                   crop_threshold_frames, crop_threshold_pixels,
                                   exclude_extensions, hw_decode, keep_duplicate_titles);
}

void hb_force_rescan( hb_handle_t * h )
{
    free((char*)h->title_set.path);
    h->title_set.path = NULL;
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

int hb_save_preview( hb_handle_t * h, int title, int preview, hb_buffer_t *buf, int format )
{
    FILE    * file;
    char    * filename;
    char      reason[80];
    const int planes_max   = 3;
    const int format_chars = 4;
    char      format_string[format_chars];

    switch (format)
    {
        case HB_PREVIEW_FORMAT_YUV:
            strncpy(format_string, "yuv", format_chars);
            break;
        case HB_PREVIEW_FORMAT_JPG:
            strncpy(format_string, "jpg", format_chars);
            break;
        default:
            hb_error("hb_save_preview: Unsupported preview format %d", format);
            return -1;
    }

    filename = hb_get_temporary_filename("%d_%d_%d.%s", hb_get_instance_id(h),
                                         title, preview, format_string);

    file = hb_fopen(filename, "wb");
    if (file == NULL)
    {
        if (strerror_r(errno, reason, 79) != 0)
        {
            strcpy(reason, "unknown -- strerror_r() failed");
        }
        hb_error("hb_save_preview: Failed to open %s (reason: %s)",
                 filename, reason);
        free(filename);
        return -1;
    }

    if (format == HB_PREVIEW_FORMAT_YUV)
    {
        int pp, hh;
        for(pp = 0; pp < planes_max; pp++)
        {
            const uint8_t * data = buf->plane[pp].data;
            const int     stride = buf->plane[pp].stride;
            const int          w = buf->plane[pp].width;
            const int          h = buf->plane[pp].height;

            for(hh = 0; hh < h; hh++)
            {
                if (fwrite(data, w, 1, file) < w)
                {
                    if (ferror(file))
                    {
                        if (strerror_r(errno, reason, 79) != 0)
                        {
                            strcpy(reason, "unknown -- strerror_r() failed");
                        }
                        hb_error("hb_save_preview: Failed to write line %d to %s "
                                 "(reason: %s). Preview will be incomplete.",
                                 hh, filename, reason);
                        goto done;
                    }
                }
                data += stride;
            }
        }
    }
    else if (format == HB_PREVIEW_FORMAT_JPG)
    {
        tjhandle        jpeg_compressor = tjInitCompress();
        const int       jpeg_quality = 90;
        unsigned long   jpeg_size    = 0;
        unsigned char * jpeg_data    = NULL;
        int             planes_stride[planes_max];
        uint8_t       * planes_data[planes_max];
        int             pp, compressor_result;
        for (pp = 0; pp < planes_max; pp++)
        {
            planes_stride[pp] = buf->plane[pp].stride;
            planes_data[pp]   = buf->plane[pp].data;
        }

        compressor_result = tjCompressFromYUVPlanes(jpeg_compressor,
                                                    (const unsigned char **)planes_data,
                                                    buf->plane[0].width,
                                                    planes_stride,
                                                    buf->plane[0].height,
                                                    TJSAMP_420,
                                                    &jpeg_data,
                                                    &jpeg_size,
                                                    jpeg_quality,
                                                    TJFLAG_FASTDCT);
        if (compressor_result == 0)
        {
            const size_t ret = fwrite(jpeg_data, jpeg_size, 1, file);
            if ((ret < jpeg_size) && (ferror(file)))
            {
                if (strerror_r(errno, reason, 79) != 0)
                {
                    strcpy(reason, "unknown -- strerror_r() failed");
                }
                hb_error("hb_save_preview: Failed to write to %s "
                         "(reason: %s).", filename, reason);
            }
        }
        else
        {
            hb_error("hb_save_preview: JPEG compression failed for "
                     "preview image %s", filename);
        }

        tjDestroy(jpeg_compressor);
        tjFree(jpeg_data);
    }

done:
    free(filename);
    fclose(file);

    return 0;
}

hb_buffer_t * hb_read_preview(hb_handle_t * h, hb_title_t *title, int preview, int format)
{
    FILE    * file = NULL;
    char    * filename = NULL;
    char      reason[80];
    const int planes_max   = 3;
    const int format_chars = 4;
    char      format_string[format_chars];

    hb_buffer_t * buf;
    buf = hb_frame_buffer_init(AV_PIX_FMT_YUV420P,
                               title->geometry.width, title->geometry.height);
    buf->f.color_prim      = title->color_prim;
    buf->f.color_transfer  = title->color_transfer;
    buf->f.color_matrix    = title->color_matrix;
    buf->f.color_range     = AVCOL_RANGE_MPEG;
    buf->f.chroma_location = title->chroma_location;

    if (!buf)
    {
        hb_error("hb_read_preview: hb_frame_buffer_init failed");
        goto done;
    }

    switch (format)
    {
        case HB_PREVIEW_FORMAT_YUV:
            strncpy(format_string, "yuv", format_chars);
            break;
        case HB_PREVIEW_FORMAT_JPG:
            strncpy(format_string, "jpg", format_chars);
            break;
        default:
            hb_error("hb_read_preview: Unsupported preview format %d", format);
            return buf;
    }

    filename = hb_get_temporary_filename("%d_%d_%d.%s", hb_get_instance_id(h),
                                         title->index, preview, format_string);

    file = hb_fopen(filename, "rb");
    if (file == NULL)
    {
        if (strerror_r(errno, reason, 79) != 0)
        {
            strcpy(reason, "unknown -- strerror_r() failed");
        }
        hb_error("hb_read_preview: Failed to open %s (reason: %s)",
                 filename, reason);
        free(filename);
        return NULL;
    }

    if (format == HB_PREVIEW_FORMAT_YUV)
    {
        int pp, hh;
        for (pp = 0; pp < planes_max; pp++)
        {
            uint8_t       * data = buf->plane[pp].data;
            const int     stride = buf->plane[pp].stride;
            const int          w = buf->plane[pp].width;
            const int          h = buf->plane[pp].height;

            for (hh = 0; hh < h; hh++)
            {
                if (fread(data, w, 1, file) < w)
                {
                    if (ferror(file))
                    {
                        if (strerror_r(errno, reason, 79) != 0)
                        {
                            strcpy(reason, "unknown -- strerror_r() failed");
                        }
                        hb_error("hb_read_preview: Failed to read line %d from %s "
                                 "(reason: %s). Preview will be incomplete.",
                                 hh, filename, reason );
                        goto done;
                    }
                }
                data += stride;
            }
        }
    }
    else if (format == HB_PREVIEW_FORMAT_JPG)
    {
        fseek(file, 0, SEEK_END);
        unsigned long jpeg_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        unsigned char * jpeg_data = tjAlloc(jpeg_size + 1);
        jpeg_data[jpeg_size] = 0;

        const size_t ret = fread(jpeg_data, jpeg_size, 1, file);
        {
            if ((ret < jpeg_size) && (ferror(file)))
            {
                if (strerror_r(errno, reason, 79) != 0)
                {
                    strcpy(reason, "unknown -- strerror_r() failed");
                }
                hb_error("hb_read_preview: Failed to read from %s "
                         "(reason: %s).", filename, reason);
                tjFree(jpeg_data);
                goto done;
            }
        }

        tjhandle   jpeg_decompressor = tjInitDecompress();
        int        planes_stride[planes_max];
        uint8_t  * planes_data[planes_max];
        int        pp, decompressor_result;
        for (pp = 0; pp < planes_max; pp++)
        {
            planes_stride[pp] = buf->plane[pp].stride;
            planes_data[pp]   = buf->plane[pp].data;
        }

        decompressor_result = tjDecompressToYUVPlanes(jpeg_decompressor,
                                                      jpeg_data,
                                                      jpeg_size,
                                                      (unsigned char **)planes_data,
                                                      buf->plane[0].width,
                                                      planes_stride,
                                                      buf->plane[0].height,
                                                      TJFLAG_FASTDCT);
        if (decompressor_result != 0)
        {
            hb_error("hb_read_preview: JPEG decompression failed for "
                     "preview image %s", filename);
        }

        tjDestroy(jpeg_decompressor);
        tjFree(jpeg_data);
    }

done:
    free(filename);
    fclose(file);

    return buf;
}

static void process_filter(hb_filter_object_t * filter)
{
    hb_buffer_t * in, * out;

    while (filter->status != HB_FILTER_DONE)
    {
        in = hb_fifo_get_wait(filter->fifo_in);
        if (in == NULL)
            continue;

        out = NULL;
        filter->status = filter->work(filter, &in, &out);
        if (in != NULL)
        {
            hb_buffer_close(&in);
        }
        if (out != NULL)
        {
            hb_fifo_push(filter->fifo_out, out);
        }
    }
}

// Get preview and apply applicable filters
hb_image_t * hb_get_preview3(hb_handle_t * h, int picture,
                             hb_dict_t * job_dict)
{
    hb_job_t    * job;
    hb_title_t  * title = NULL;
    hb_buffer_t * in = NULL, * out;

    job = hb_dict_to_job(h, job_dict);
    if (job == NULL)
    {
        hb_error("hb_get_preview3: failed to unpack job");
        goto fail;
    }
    title = job->title;

    in = hb_read_preview( h, title, picture, HB_PREVIEW_FORMAT_JPG );
    if (in == NULL)
    {
        goto fail;
    }

    // Initialize supported filters
    hb_list_t        * list_filter = job->list_filter;
    hb_filter_init_t   init;
    int                ii;

    memset(&init, 0, sizeof(init));
    init.time_base.num = 1;
    init.time_base.den = 90000;
    init.job = job;
    init.pix_fmt = AV_PIX_FMT_YUV420P;
    init.hw_pix_fmt = AV_PIX_FMT_NONE;
    init.color_range = AVCOL_RANGE_MPEG;

    init.color_prim = title->color_prim;
    init.color_transfer = title->color_transfer;
    init.color_matrix = title->color_matrix;
    init.chroma_location = title->chroma_location;
    init.geometry = title->geometry;
    memset(init.crop, 0, sizeof(int[4]));
    init.vrate = job->vrate;
    init.cfr = 0;
    init.grayscale = 0;

    hb_filter_object_t * filter;

    for (ii = 0; ii < hb_list_count(list_filter); )
    {
        filter = hb_list_item(list_filter, ii);
        switch (filter->id)
        {
            case HB_FILTER_AVFILTER:
            case HB_FILTER_CROP_SCALE:
            case HB_FILTER_PAD:
            case HB_FILTER_ROTATE:
            case HB_FILTER_COLORSPACE:
            case HB_FILTER_DECOMB:
            case HB_FILTER_DETELECINE:
            case HB_FILTER_YADIF:
            case HB_FILTER_GRAYSCALE:
                break;

            case HB_FILTER_VFR:
            case HB_FILTER_RENDER_SUB:
            case HB_FILTER_NLMEANS:
            case HB_FILTER_CHROMA_SMOOTH:
            case HB_FILTER_LAPSHARP:
            case HB_FILTER_UNSHARP:
            case HB_FILTER_DEBLOCK:
            case HB_FILTER_COMB_DETECT:
            case HB_FILTER_HQDN3D:
            case HB_FILTER_BWDIF:
                // Not implemented, N/A, or requires multiple frame input
                hb_list_rem(list_filter, filter);
                hb_filter_close(&filter);
                continue;
            default:
                hb_log("hb_get_preview3: Unrecognized filter (%d)",
                       filter->id);
                hb_list_rem(list_filter, filter);
                hb_filter_close(&filter);
                continue;
        }
        if (filter->init != NULL && filter->init(filter, &init))
        {
            hb_error("hb_get_preview3: Failure to initialize filter '%s'",
                     filter->name);
            hb_list_rem(list_filter, filter);
            hb_filter_close(&filter);
            continue;
        }
        ii++;
    }

    job->output_pix_fmt = init.pix_fmt;
    job->color_prim = init.color_prim;
    job->color_transfer = init.color_transfer;
    job->color_matrix = init.color_matrix;
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

    // Add "cropscale"
    // Adjusts for pixel aspect, performs any requested
    // post-scaling and sets required pix_fmt AV_PIX_FMT_RGB32
    //
    // This will scale the result at the end of the pipeline.
    // I.e. padding will be scaled
    hb_rational_t par = job->par;

    int scaled_width  = init.geometry.width;
    int scaled_height = init.geometry.height;

    filter = hb_filter_init(HB_FILTER_CROP_SCALE);
    filter->settings = hb_dict_init();
    if (par.num >= par.den)
    {
        scaled_width = scaled_width * par.num / par.den;
    }
    else
    {
        scaled_height = scaled_height * par.den / par.num;
    }
    hb_dict_set_int(filter->settings, "width", scaled_width);
    hb_dict_set_int(filter->settings, "height", scaled_height);
    hb_list_add(job->list_filter, filter);

    if (filter->init != NULL && filter->init(filter, &init))
    {
        hb_error("hb_get_preview3: Failure to initialize filter '%s'",
                 filter->name);
        hb_list_rem(list_filter, filter);
        hb_filter_close(&filter);
    }

    filter = hb_filter_init(HB_FILTER_FORMAT);
    filter->settings = hb_dict_init();
    hb_dict_set_string(filter->settings, "format", av_get_pix_fmt_name(AV_PIX_FMT_RGB32));
    hb_list_add(job->list_filter, filter);

    if (filter->init != NULL && filter->init(filter, &init))
    {
        hb_error("hb_get_preview3: Failure to initialize filter '%s'",
                 filter->name);
        hb_list_rem(list_filter, filter);
        hb_filter_close(&filter);
    }

    hb_avfilter_combine(list_filter);

    for( ii = 0; ii < hb_list_count( list_filter ); )
    {
        filter = hb_list_item( list_filter, ii );
        filter->done = &job->done;
        if (filter->post_init != NULL && filter->post_init(filter, job))
        {
            hb_log( "hb_get_preview3: Failure to initialise filter '%s'",
                    filter->name );
            hb_list_rem(list_filter, filter);
            hb_filter_close(&filter);
            continue;
        }
        ii++;
    }

    // Set up filter fifos
    hb_fifo_t *fifo_in, * fifo_first, * fifo_last;

    fifo_last = fifo_in = fifo_first = hb_fifo_init(2, 2);
    for( ii = 0; ii < hb_list_count( list_filter ); ii++)
    {
        filter = hb_list_item(list_filter, ii);
        if (!filter->skip)
        {
            filter->fifo_in = fifo_in;
            filter->fifo_out = hb_fifo_init(2, 2);
            fifo_last = fifo_in = filter->fifo_out;
        }
    }

    // Feed preview frame to filter chain
    hb_fifo_push(fifo_first, in);
    hb_fifo_push(fifo_first, hb_buffer_eof_init());

    // Process the preview frame through all filters
    for( ii = 0; ii < hb_list_count( list_filter ); ii++)
    {
        filter = hb_list_item(list_filter, ii);
        if (!filter->skip)
        {
            process_filter(filter);
        }
    }
    // Retrieve the filtered preview frame
    out = hb_fifo_get(fifo_last);

    // Close filters
    for (ii = 0; ii < hb_list_count(list_filter); ii++)
    {
        filter = hb_list_item(list_filter, ii);
        filter->close(filter);
    }

    // Close fifos
    hb_fifo_close(&fifo_first);
    for( ii = 0; ii < hb_list_count( list_filter ); ii++)
    {
        filter = hb_list_item(list_filter, ii);
        hb_fifo_close(&filter->fifo_out);
    }

    if (out == NULL)
    {
        hb_error("hb_get_preview3: Failed to filter preview");
        goto fail;
    }

    hb_image_t *image = hb_buffer_to_image(out);
    hb_buffer_close(&out);
    if (image->width < 16 || image->height < 16)
    {
        // Guard against broken filter generating degenerate images
        hb_error("hb_get_preview3: bad preview image output by filters");
        hb_image_close(&image);
        goto fail;
    }

    // Clean up
    hb_job_close(&job);

    return image;

fail:

    {
        int width = 854, height = 480;

        if (title != NULL)
        {
            hb_geometry_t * geo = &title->geometry;

            width = geo->width * geo->par.num / geo->par.den;
            height = geo->height;
        }

        image = hb_image_init(AV_PIX_FMT_RGB32, width, height);
    }
    hb_job_close(&job);

    return image;
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
    int j, k, n, off, cc_1, cc_2, cc[3] = {0};
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
    for( k = 0; k <= buf->f.max_plane; k++ )
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
            Because sensitivity goes down to hundredths of a percent, multiply by 1000
            so it will be easy to compare against the threshold value which is an integer. */
        cc[k] = (int)( ( cc_1 + cc_2 ) * 1000.0 / ( width * height ) );
    }


    /* HandBrake previews are all yuv420, so weight the average percentage of all 3 planes accordingly. */
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

static void hflip_crop_pad(int * dst, int * src, int hflip)
{
    if (hflip)
    {
        dst[2] = src[3];
        dst[3] = src[2];
    }
}

static void rotate_crop_pad(int * dst, int * src, int angle)
{
    if (angle == 90)
    {
        dst[3] = src[0];
        dst[1] = src[3];
        dst[2] = src[1];
        dst[0] = src[2];
    }
    else if (angle == 180)
    {
        dst[0] = src[1];
        dst[1] = src[0];
        dst[2] = src[3];
        dst[3] = src[2];
    }
    else if (angle == 270)
    {
        dst[2] = src[0];
        dst[1] = src[2];
        dst[3] = src[1];
        dst[0] = src[3];
    }
}

void hb_rotate_geometry( hb_geometry_crop_t * geo, hb_geometry_crop_t * result,
                         int angle, int hflip)
{
    hb_geometry_crop_t in = *geo; // Make a copy in case geo aliases result
    *result = in;

    // re-orient result if needed
    hflip_crop_pad(result->crop, in.crop, hflip);
    hflip_crop_pad(result->pad,  in.pad,  hflip);
    if (angle == 90 || angle == 270)
    {
        result->geometry.width   = in.geometry.height;
        result->geometry.height  = in.geometry.width;
        result->geometry.par.num = in.geometry.par.den;
        result->geometry.par.den = in.geometry.par.num;
    }
    in = *result;
    rotate_crop_pad(result->crop, in.crop, angle);
    rotate_crop_pad(result->pad,  in.pad,  angle);
}

/**
 * Calculates destination width and height for anamorphic content
 *
 * Returns calculated geometry
 * @param source_geometry - Pointer to source geometry info
 * @param geometry        - Pointer to requested destination parameters
 */
void hb_set_anamorphic_size2(hb_geometry_t          * src_geo,
                             hb_geometry_settings_t * geo,
                             hb_geometry_t          * result)
{
    hb_rational_t in_par, src_par, orig_par;
    int keep_display_aspect = !!(geo->keep & HB_KEEP_DISPLAY_ASPECT);
    int keep_display_width  = !!(geo->keep & HB_KEEP_DISPLAY_WIDTH);
    int keep_height         = !!(geo->keep & HB_KEEP_HEIGHT);
    int keep_width          = !!(geo->keep & HB_KEEP_WIDTH);
    int keep_pad            = !!(geo->keep & HB_KEEP_PAD);
    int upscale             = !!(geo->flags & HB_GEO_SCALE_UP);
    int best                = !!(geo->flags & HB_GEO_SCALE_BEST);

    /* Set up some variables to make the math easier to follow. */
    int    cropped_width  = src_geo->width  - geo->crop[2] - geo->crop[3];
    int    cropped_height = src_geo->height - geo->crop[0] - geo->crop[1];
    double cropped_sar    = (double)cropped_width / cropped_height;
    int    mod            = (geo->modulus > 0) ? EVEN(geo->modulus) : 2;
    int    pad_width      = geo->pad[2] + geo->pad[3];
    int    pad_height     = geo->pad[0] + geo->pad[1];
    double displayWidth, displayHeight;

    // Sanitize PAR
    if (geo->geometry.par.num == 0 || geo->geometry.par.den == 0)
    {
        geo->geometry.par.num = geo->geometry.par.den = 1;
    }
    if (src_geo->par.num == 0 || src_geo->par.den == 0)
    {
        src_geo->par.num = src_geo->par.den = 1;
    }

    in_par  = geo->geometry.par;
    src_par = src_geo->par;

    if (!keep_width && !keep_display_width)
    {
        orig_par.num = geo->displayWidth;
        orig_par.den = geo->geometry.width + pad_width;
    }
    else
    {
        orig_par = in_par;
    }

    /* If a source was really NTSC or PAL and the user specified ITU PAR
       values, replace the standard PAR values with the ITU broadcast ones. */
    if (src_geo->width == 720 && geo->itu_par)
    {
        // convert aspect to a scaled integer so we can test for 16:9 & 4:3
        // aspect ratios ignoring insignificant differences in the LSBs of
        // the floating point representation.
        int iaspect = src_geo->width * src_par.num * 9. /
                      (src_geo->height * src_par.den);

        /* Handle ITU PARs */
        if (src_geo->height == 480)
        {
            /* It's NTSC */
            if (iaspect == 16)
            {
                /* It's widescreen */
                in_par.num = 40;
                in_par.den = 33;
            }
            else if (iaspect == 12)
            {
                /* It's 4:3 */
                in_par.num = 10;
                in_par.den = 11;
            }
        }
        else if (src_geo->height == 576)
        {
            /* It's PAL */
            if (iaspect == 16)
            {
                /* It's widescreen */
                in_par.num = 16;
                in_par.den = 11;
            }
            else if (iaspect == 12)
            {
                /* It's 4:3 */
                in_par.num = 12;
                in_par.den = 11;
            }
        }
    }


    // Remove pad from DAR, simplifies latter calculations
    displayWidth = geo->displayWidth -
                   (double)pad_width * orig_par.den / orig_par.num;
    displayHeight = geo->displayHeight - pad_height;
    if (!keep_pad)
    {

        pad_width  = 0;
        pad_height = 0;
    }

    int width, height;
    int maxWidth, maxHeight;

    if (geo->maxWidth > 0)
    {
        maxWidth  = MIN(MAX(MULTIPLE_MOD_DOWN(geo->maxWidth, mod),
                            HB_MIN_WIDTH), HB_MAX_WIDTH);
    }
    else
    {
        maxWidth  = HB_MAX_WIDTH;
    }
    if (geo->maxHeight > 0)
    {
        maxHeight = MIN(MAX(MULTIPLE_MOD_DOWN(geo->maxHeight, mod),
                            HB_MIN_HEIGHT), HB_MAX_HEIGHT);
    }
    else
    {
        maxHeight = HB_MAX_HEIGHT;
    }
    if (!upscale)
    {
        // Limit resolution to source resolution + pad
        if (maxWidth > cropped_width + pad_width)
        {
            maxWidth = MULTIPLE_MOD_DOWN(cropped_width + pad_width, mod);
        }
        if (maxHeight > cropped_height + pad_height)
        {
            maxHeight = MULTIPLE_MOD_DOWN(cropped_height + pad_height, mod);
        }
    }

    width  = geo->geometry.width;
    height = geo->geometry.height;

    if (best && !keep_display_width)
    {
        // Select "best" resolution based on upscale settings and
        // source resolution.
        width  = cropped_width;
        height = cropped_height;
        if (upscale && maxWidth > 0)
        {
            width = maxWidth - pad_width;
        }
        if (upscale && maxHeight > 0)
        {
            height = maxHeight - pad_height;
        }
    }
    switch (geo->mode)
    {
        case HB_ANAMORPHIC_STRICT:
            /* "Strict" anamorphic: MOD 2, source PAR, and source dimensions */
            mod    = 2;
            in_par = src_par;
            width  = cropped_width;
            height = cropped_height;
            break;

        case HB_ANAMORPHIC_LOOSE:
            in_par = src_par;
            break;

        case HB_ANAMORPHIC_NONE:
            /* "None" anamorphic, a.k.a. 1:1.  */
            in_par.num = 1;
            in_par.den = 1;
            break;

        case HB_ANAMORPHIC_CUSTOM:
            /* "Custom" anamorphic: Set a specific PAR */
        default:
            break;
    }

    // Use 64 bits to avoid overflow till the final hb_reduce() call
    int64_t dst_par_num;
    int64_t dst_par_den;

    hb_reduce64(&dst_par_num, &dst_par_den, in_par.num, in_par.den);

    switch (geo->mode)
    {
        case HB_ANAMORPHIC_STRICT:
        case HB_ANAMORPHIC_LOOSE:
        case HB_ANAMORPHIC_NONE:
        case HB_ANAMORPHIC_CUSTOM:
        {
            double dar;

            if (keep_display_aspect)
            {
                // Recompute width or height to maintain the source DAR
                double par;

                par = (double)src_par.num / src_par.den;
                dar = par * cropped_sar;

                if (!keep_height)
                {
                    width = MULTIPLE_MOD_UP(width, mod);
                    height = MULTIPLE_MOD(
                        width * dst_par_num / dst_par_den / dar, mod);
                }
                else
                {
                    height = MULTIPLE_MOD_UP(height, mod);
                    width  = MULTIPLE_MOD_UP(
                        dar * height * dst_par_den / dst_par_num, mod);
                }
            }
            else if (keep_display_width)
            {
                // Recompute width if we are modifying display size
                // in these anamorphic modes. These modes all have a fixed
                // user specified PAR.
                //
                // It is assumed displayHeight == height in this scenario.
                double dwidth;
                dar = (double)displayWidth / displayHeight;

                // We need to determine if we should round up or down.
                // If a wider display aspect is requested, then the computed
                // 'double' storage aspect will be larger than the current
                // storage aspect, else it will be smaller.
                dwidth = displayWidth * dst_par_den / dst_par_num;
                if (dwidth > geo->geometry.width)
                {
                    width = MULTIPLE_MOD_UP((int)dwidth, mod);
                }
                else
                {
                    width = MULTIPLE_MOD_DOWN((int)dwidth, mod);
                }
                height = MULTIPLE_MOD_UP(height, mod);
            }
            else
            {
                // Recompute width or height to maintain the current DAR
                dar = (double)displayWidth / displayHeight;
                if (!keep_height)
                {
                    width = MULTIPLE_MOD_UP(width, mod);
                    height = MULTIPLE_MOD(
                        width * dst_par_num / dst_par_den / dar, mod);
                }
                else
                {
                    height = MULTIPLE_MOD_UP(height, mod);
                    width  = MULTIPLE_MOD(
                        dar * height * dst_par_den / dst_par_num, mod);
                }
            }

            // Limit to min/max dimensions
            if (width + pad_width > maxWidth)
            {
                width  = maxWidth - pad_width;
                if (keep_display_aspect)
                {
                    height = MULTIPLE_MOD(
                        width * dst_par_num / dst_par_den / dar, mod);
                }
            }
            if (height + pad_height > maxHeight)
            {
                height  = maxHeight - pad_height;
                if (keep_display_aspect)
                {
                    width  = MULTIPLE_MOD_UP(
                        dar * height * dst_par_den / dst_par_num, mod);
                }
            }
            if (width < HB_MIN_WIDTH)
            {
                width  = HB_MIN_WIDTH;
                if (keep_display_aspect)
                {
                    height = MULTIPLE_MOD(
                        width * dst_par_num / dst_par_den / dar, mod);
                }
            }
            if (height < HB_MIN_HEIGHT)
            {
                height  = HB_MIN_HEIGHT;
                if (keep_display_aspect)
                {
                    width  = MULTIPLE_MOD_UP(
                        dar * height * dst_par_den / dst_par_num, mod);
                }
            }

            if (geo->mode != HB_ANAMORPHIC_CUSTOM &&
                geo->mode != HB_ANAMORPHIC_NONE && keep_display_aspect)
            {
                // Rounding errors in width/height calculations may
                // throw off final display aspect.
                //
                // Recompute PAR to fix display aspect, unless user set
                // a specific PAR.
                dst_par_num = (int64_t)height * cropped_width  * src_par.num;
                dst_par_den = (int64_t)width  * cropped_height * src_par.den;
            }
        } break;

        default:
        case HB_ANAMORPHIC_AUTO:
        {
            /* "Automatic" anamorphic.
             *  - Uses mod-compliant dimensions, set by user
             *  - Allows users to set the either width *or* height
             *  - Does *not* maintain original source PAR if one
             *    or both dimensions is limited by maxWidth/maxHeight.
             */

            if (!keep_display_aspect && keep_display_width)
            {
                // Recompute PAR if we are modifying display size
                // in auto anamorphic mode (stretching the display aspect).
                //
                // Not that (keep_display_aspect && geo->displayWidth < 0)
                // is not handled.  It is not a valid use case in the
                // frontends, so it is unnecessary to complicate this further
                // with support for it.
                width  = geo->geometry.width;
                height = geo->geometry.height;
                dst_par_num = displayWidth;
                dst_par_den = width;
                break;
            }

            /* Time to get picture dimensions that divide cleanly.*/
            width  = MULTIPLE_MOD_UP(width, mod);
            height = MULTIPLE_MOD_UP(height, mod);

            // Limit to min/max dimensions
            if (width + pad_width > maxWidth)
            {
                width = maxWidth - pad_width;
            }
            if (height + pad_height > maxHeight)
            {
                height = maxHeight - pad_height;
            }
            if (width < HB_MIN_WIDTH)
            {
                width  = HB_MIN_WIDTH;
            }
            if (height < HB_MIN_HEIGHT)
            {
                height  = HB_MIN_HEIGHT;
            }

            /* Adjust the output PAR for new width/height */
            if (keep_display_aspect)
            {
                dst_par_num = (int64_t)height * cropped_width  * src_par.num;
                dst_par_den = (int64_t)width  * cropped_height * src_par.den;
            }
            else
            {
                dst_par_num = displayWidth;
                dst_par_den = width;
            }
        } break;
    }

    /* Pass the results back to the caller */
    result->width  = width;
    result->height = height;

    // If the user is directly updating PAR, don't override PAR values.
    // I.e. don't even reduce the values.
    if (geo->mode == HB_ANAMORPHIC_CUSTOM ||
        geo->mode == HB_ANAMORPHIC_NONE)
    {
        // In these 2 anamorphic modes, the result PAR should be exactly
        // what the user set PAR to.
        result->par = in_par;
    }
    else
    {
        /* While x264 is smart enough to reduce fractions on its own, libavcodec
         * needs some help with the math, so lose superfluous factors. */
        hb_limit_rational64(&dst_par_num, &dst_par_den,
                            dst_par_num, dst_par_den, 65535);

        hb_reduce(&result->par.num, &result->par.den, dst_par_num, dst_par_den);
    }
}

/**
 * Add a filter to a jobs filter list
 *
 * @param job Handle to hb_job_t
 * @param settings to give the filter
 */
void hb_add_filter2( hb_value_array_t * list, hb_dict_t * filter_dict )
{
    int new_id = hb_dict_get_int(filter_dict, "ID");

    hb_filter_object_t * filter = hb_filter_get(new_id);
    if (filter == NULL)
    {
        hb_error("hb_add_filter2: Invalid filter ID %d", new_id);
        hb_value_free(&filter_dict);
        return;
    }
    if (filter->enforce_order)
    {
        // Find the position in the filter chain this filter belongs in
        int ii, len;

        len = hb_value_array_len(list);
        for( ii = 0; ii < len; ii++ )
        {
            hb_value_t * f = hb_value_array_get(list, ii);
            int id = hb_dict_get_int(f, "ID");
            if (id > new_id)
            {
                hb_value_array_insert(list, ii, filter_dict);
                return;
            }
            else if ( id == new_id )
            {
                // Updating this filter with new settings
                hb_value_array_set(list, ii, filter_dict);
                return;
            }
        }
    }
    // No position found or order not enforced for this filter
    hb_value_array_append(list, filter_dict);
}

/**
 * Add a filter to a jobs filter list
 *
 * @param job Handle to hb_job_t
 * @param settings to give the filter
 */
void hb_add_filter_dict( hb_job_t * job, hb_filter_object_t * filter,
                         const hb_dict_t * settings_in )
{
    if (filter == NULL)
    {
        hb_log("hb_add_filter_dict: filter is null");
        return;
    }
    hb_dict_t * settings;

    // Always set filter->settings to a valid hb_dict_t
    if (settings_in == NULL)
    {
        settings = hb_dict_init();
    }
    else
    {
        settings = hb_value_dup(settings_in);
    }
    filter->settings = settings;
    if (filter->sub_filter)
    {
        filter->sub_filter->settings = hb_value_dup(settings);
    }
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
 * Add a filter to a jobs filter list
 *
 * @param job Handle to hb_job_t
 * @param settings to give the filter
 */
void hb_add_filter( hb_job_t * job, hb_filter_object_t * filter,
                    const char * settings_in )
{
    if (filter == NULL)
    {
        hb_log("hb_add_filter: filter is null");
        return;
    }
    hb_dict_t * settings = hb_parse_filter_settings(settings_in);
    if (settings_in != NULL && settings == NULL)
    {
        hb_log("hb_add_filter: failed to parse filter settings!");
        return;
    }
    hb_add_filter_dict(job, filter, settings);
    hb_value_free(&settings);
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
static void hb_add_internal( hb_handle_t * h, hb_job_t * job, hb_list_t *list_pass )
{
    hb_job_t      * job_copy;
    hb_audio_t    * audio;
    hb_subtitle_t * subtitle;
    int             i;
    char            audio_lang[4];

    /* Copy the job */
    job_copy                  = calloc( sizeof( hb_job_t ), 1 );
    memcpy(job_copy, job, sizeof(hb_job_t));
    job_copy->json            = NULL;
    job_copy->encoder_preset  = NULL;
    job_copy->encoder_tune    = NULL;
    job_copy->encoder_profile = NULL;
    job_copy->encoder_level   = NULL;
    job_copy->encoder_options = NULL;
    job_copy->file            = NULL;
    job_copy->list_chapter    = NULL;
    job_copy->list_audio      = NULL;
    job_copy->list_subtitle   = NULL;
    job_copy->list_filter     = NULL;
    job_copy->list_attachment = NULL;
    job_copy->metadata        = NULL;

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
        int count = hb_list_count(job_copy->list_subtitle);
        if (count == 0 ||
            (count == 1 && !job_copy->select_subtitle_config.force))
        {
            hb_log("Skipping subtitle scan.  No suitable subtitle tracks.");
            hb_job_close(&job_copy);
            return;
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

    if (job->encoder_preset != NULL)
        job_copy->encoder_preset = strdup(job->encoder_preset);
    if (job->encoder_tune != NULL)
        job_copy->encoder_tune = strdup(job->encoder_tune);
    if (job->encoder_options != NULL)
        job_copy->encoder_options = strdup(job->encoder_options);
    if (job->encoder_profile != NULL)
        job_copy->encoder_profile = strdup(job->encoder_profile);
    if (job->encoder_level != NULL)
        job_copy->encoder_level = strdup(job->encoder_level);
    if (job->file != NULL)
        job_copy->file = strdup(job->file);

    job_copy->h     = h;

    /* Copy the job filter list */
    job_copy->list_filter = hb_filter_list_copy( job->list_filter );

    /* Add the job to the list */
    hb_list_add( list_pass, job_copy );
}

hb_job_t* hb_job_copy(hb_job_t * job)
{
    hb_job_t      * job_copy;

    /* Copy the job */
    job_copy        = calloc( sizeof( hb_job_t ), 1 );
    if (job_copy == NULL)
        return NULL;

    if (job->json != NULL)
    {
        // JSON jobs should only have the json string set.
        job_copy->json = strdup(job->json);
        return job_copy;
    }
    memcpy( job_copy, job, sizeof( hb_job_t ) );

    job_copy->list_subtitle = hb_subtitle_list_copy( job->list_subtitle );
    job_copy->list_chapter = hb_chapter_list_copy( job->list_chapter );
    job_copy->list_audio = hb_audio_list_copy( job->list_audio );
    job_copy->list_attachment = hb_attachment_list_copy( job->list_attachment );
    job_copy->metadata = hb_metadata_copy( job->metadata );

    if (job->encoder_preset != NULL)
        job_copy->encoder_preset = strdup(job->encoder_preset);
    if (job->encoder_tune != NULL)
        job_copy->encoder_tune = strdup(job->encoder_tune);
    if (job->encoder_options != NULL)
        job_copy->encoder_options = strdup(job->encoder_options);
    if (job->encoder_profile != NULL)
        job_copy->encoder_profile = strdup(job->encoder_profile);
    if (job->encoder_level != NULL)
        job_copy->encoder_level = strdup(job->encoder_level);
    if (job->file != NULL)
        job_copy->file = strdup(job->file);

    job_copy->list_filter = hb_filter_list_copy( job->list_filter );

    return job_copy;
}

int hb_add( hb_handle_t * h, hb_job_t * job )
{
    hb_job_t *job_copy = hb_job_copy(job);
    job_copy->h = h;
    job_copy->sequence_id = ++h->sequence_id;
    hb_list_add(h->jobs, job_copy);

    return job_copy->sequence_id;
}

void hb_job_setup_passes(hb_handle_t * h, hb_job_t * job, hb_list_t * list_pass)
{
    if (job->vquality > HB_INVALID_VIDEO_QUALITY && ! hb_video_multipass_is_supported(job->vcodec, 1))
    {
        job->multipass = 0;
    }
    if (job->indepth_scan)
    {
        hb_deep_log(2, "Adding subtitle scan pass");
        job->pass_id = HB_PASS_SUBTITLE;
        hb_add_internal(h, job, list_pass);
        job->indepth_scan = 0;
    }
    if (job->multipass)
    {
        hb_deep_log(2, "Adding multi-pass encode");
        int analysis_pass_count = hb_video_encoder_get_count_of_analysis_passes(job->vcodec);
        for (int i = 0; i < analysis_pass_count; i++)
        {
            job->pass_id = HB_PASS_ENCODE_ANALYSIS;
            hb_add_internal(h, job, list_pass);
        }
        job->pass_id = HB_PASS_ENCODE_FINAL;
        hb_add_internal(h, job, list_pass);
    }
    else
    {
        job->pass_id = HB_PASS_ENCODE;
        hb_add_internal(h, job, list_pass);
    }
}

/**
 * Removes a job from the job list.
 * @param h Handle to hb_handle_t.
 * @param job Handle to hb_job_t.
 */
void hb_rem( hb_handle_t * h, hb_job_t * job )
{
    hb_list_rem( h->jobs, job );
}

/**
 * Starts the conversion process.
 * Sets state to HB_STATE_WORKING.
 * calls hb_work_init, to launch work thread. Stores handle to work thread.
 * @param h Handle to hb_handle_t.
 */
void hb_start( hb_handle_t * h )
{
    hb_lock( h->state_lock );
    h->state.state       = HB_STATE_WORKING;
    h->state.sequence_id = 0;
#define p h->state.param.working
    p.pass         = -1;
    p.pass_count   = -1;
    p.progress     = 0.0;
    p.rate_cur     = 0.0;
    p.rate_avg     = 0.0;
    p.eta_seconds  = 0;
    p.hours        = -1;
    p.minutes      = -1;
    p.seconds      = -1;
    p.paused       = 0;
#undef p
    hb_unlock( h->state_lock );

    h->paused         = 0;
    h->pause_date     = -1;
    h->pause_duration = 0;
    h->work_die       = 0;
    h->work_error     = HB_ERROR_NONE;
    h->work_thread    = hb_work_init( h->jobs, &h->work_die, &h->work_error, &h->current_job );
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

        h->pause_date = hb_get_date();

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
        if (h->pause_date != -1)
        {
            // Calculate paused time for current job sequence
            h->pause_duration    += hb_get_date() - h->pause_date;

            // Calculate paused time for current job pass
            // Required to calculate accurate ETA for pass
            h->current_job->st_paused += hb_get_date() - h->pause_date;
            h->pause_date              = -1;
            h->state.param.working.paused = h->pause_duration;
        }

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
    h->work_error = HB_ERROR_CANCELED;
    h->work_die   = 1;
    hb_resume( h );
}

/**
 * Stops the conversion process.
 * @param h Handle to hb_handle_t.
 */
void hb_scan_stop( hb_handle_t * h )
{
    h->scan_die = 1;
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
 * Closes access to libhb by freeing the hb_handle_t handle contained in hb_init.
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
    free((char*)h->title_set.path);
    h->title_set.path = NULL;

    hb_list_close( &h->jobs );
    hb_lock_close( &h->state_lock );
    hb_lock_close( &h->pause_lock );

    hb_system_sleep_opaque_close(&h->system_sleep_opaque);

    free( h->interjob );

    free( h );
    *_h = NULL;
}

int hb_global_init_no_hardware()
{
    disable_hardware = 1;
    hb_log( "Init: Hardware encoders are disabled." );
    return hb_global_init();
}

int hb_global_init()
{
    /* Print hardening status on global init */
#if HB_PROJECT_SECURITY_HARDEN
    hb_log( "Compile-time hardening features are enabled" );
#endif

    int result = 0;

    result = hb_platform_init();
    if (result < 0)
    {
        hb_error("Platform specific initialization failed!");
        return -1;
    }

    /* HB work objects */
    hb_register(&hb_muxer);
    hb_register(&hb_reader);
    hb_register(&hb_sync_video);
    hb_register(&hb_sync_audio);
    hb_register(&hb_sync_subtitle);
    hb_register(&hb_decavcodecv);
    hb_register(&hb_decavcodeca);
    hb_register(&hb_declpcm);
    hb_register(&hb_decavsub);
    hb_register(&hb_decsrtsub);
    hb_register(&hb_decssasub);
    hb_register(&hb_dectx3gsub);
    hb_register(&hb_encavcodec);
    hb_register(&hb_encavcodeca);
#ifdef __APPLE__
    hb_register(&hb_encvt);
    hb_register(&hb_encca_aac);
    hb_register(&hb_encca_haac);
#endif
    hb_register(&hb_enctheora);
    hb_register(&hb_encvorbis);
    hb_register(&hb_encx264);
#if HB_PROJECT_FEATURE_X265
    hb_register(&hb_encx265);
#endif
    hb_register(&hb_encsvtav1);
#if HB_PROJECT_FEATURE_QSV
    if (!disable_hardware)
    {
        hb_register(&hb_encqsv);
    }
#endif

    hb_x264_global_init();
    hb_common_global_init(disable_hardware);

    /*
     * Initialise buffer pool
     */
    hb_buffer_pool_init();

    // Initialize the builtin presets hb_dict_t
    hb_presets_builtin_init();

    return result;
}

/**
 * Cleans up libhb at a process level. Call before the app closes. Removes preview directory.
 */
void hb_global_close()
{
    const char    * dirname;
    DIR           * dir;
    struct dirent * entry;

    hb_presets_free();

    /* Find and remove temp folder */
    dirname = hb_get_temporary_directory();

    dir = opendir( dirname );
    if (dir)
    {
        while( ( entry = readdir( dir ) ) )
        {
            char * filename;
            if( entry->d_name[0] == '.' )
            {
                continue;
            }
            filename = hb_strdup_printf("%s/%s", dirname, entry->d_name);
            unlink( filename );
            free(filename);
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
    const char * dirname;

    h->pid = getpid();

    /* Create folder for temporary files */
    dirname = hb_get_temporary_directory();

    hb_mkdir( dirname );

    while( !h->die )
    {
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
            h->state.state = HB_STATE_SCANDONE;
            hb_unlock( h->state_lock );
        }

        /* Check if the work thread is done */
        if( h->work_thread &&
            hb_thread_has_exited( h->work_thread ) )
        {
            hb_thread_close( &h->work_thread );

            hb_log( "libhb: work result = %d", h->work_error );
            hb_lock( h->state_lock );
            h->state.state               = HB_STATE_WORKDONE;
            h->state.param.working.error = h->work_error;

            hb_unlock( h->state_lock );
        }

        if (h->paused)
        {
            h->state.param.working.paused = h->pause_duration +
                                            hb_get_date() - h->pause_date;
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
    if (pipe(pfd))
       return;
#if defined( SYS_MINGW )
    // Non-console windows apps do not have a stderr->_file
    // assigned properly
    (void) freopen("NUL", "w", stderr);
    _dup2(pfd[1], _fileno(stderr));
#else
    dup2(pfd[1], STDERR_FILENO);
#endif
    setvbuf(stderr, NULL, _IONBF, 0);

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
        // Set which job is being worked on
        if (h->current_job)
            h->state.sequence_id = h->current_job->sequence_id;
    }
    hb_unlock( h->state_lock );
    hb_unlock( h->pause_lock );
}

void hb_set_work_error( hb_handle_t * h, hb_error_code err )
{
    h->work_error = err;
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

int is_hardware_disabled(void)
{
    return disable_hardware;
}
