#include "hb.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

struct hb_handle_s
{
    /* The "Check for update" thread */
    int            build;
    char           version[16];
    hb_thread_t  * update_thread;

    /* This thread's only purpose is to check other threads'
       states */
    volatile int   die;
    hb_thread_t  * main_thread;
    int            pid;

    /* DVD/file scan thread */
    hb_list_t    * list_title;
    hb_thread_t  * scan_thread;

    /* The thread which processes the jobs. Others threads are launched
       from this one (see work.c) */
    hb_list_t    * jobs;
    hb_job_t     * current_job;
    int            job_count;
    int            job_count_permanent;
    volatile int   work_die;
    int            work_error;
    hb_thread_t  * work_thread;

    int            cpu_count;

    hb_lock_t    * state_lock;
    hb_state_t     state;

    int            paused;
    hb_lock_t    * pause_lock;
    /* For MacGui active queue
       increments each time the scan thread completes*/
    int            scanCount;

};

hb_work_object_t * hb_objects = NULL;

static void thread_func( void * );

/**
 * Registers work objects, by adding the work object to a liked list.
 * @param w Handle to hb_work_object_t to register.
 */
void hb_register( hb_work_object_t * w )
{
    w->next    = hb_objects;
    hb_objects = w;
}

/**
 * libhb initialization routine.
 * @param verbose HB_DEBUG_NONE or HB_DEBUG_ALL.
 * @param update_check signals libhb to check for updated version from HandBrake website.
 * @return Handle to hb_handle_t for use on all subsequent calls to libhb.
 */
hb_handle_t * hb_init_real( int verbose, int update_check )
{
    hb_handle_t * h = calloc( sizeof( hb_handle_t ), 1 );
    uint64_t      date;

    /* See hb_deep_log() and hb_log() in common.c */
    global_verbosity_level = verbose;
    if( verbose )
        putenv( "HB_DEBUG=1" );

    /* Check for an update on the website if asked to */
    h->build = -1;

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

    /* CPU count detection */
    hb_log( "hb_init: checking cpu count" );
    h->cpu_count = hb_get_cpu_count();

    h->list_title = hb_list_init();
    h->jobs       = hb_list_init();

    h->state_lock  = hb_lock_init();
    h->state.state = HB_STATE_IDLE;

    h->pause_lock = hb_lock_init();

    /* libavcodec */
    av_register_all();

    /* Start library thread */
    hb_log( "hb_init: starting libhb thread" );
    h->die         = 0;
    h->main_thread = hb_thread_init( "libhb", thread_func, h,
                                     HB_NORMAL_PRIORITY );

    return h;

	/* Set the scan count to start at 0 */
	//scan_count = 0;
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

    /* Check for an update on the website if asked to */
    h->build = -1;

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

    /* CPU count detection */
    hb_log( "hb_init: checking cpu count" );
    h->cpu_count = hb_get_cpu_count();

    h->list_title = hb_list_init();
    h->jobs       = hb_list_init();
    h->current_job = NULL;

    h->state_lock  = hb_lock_init();
    h->state.state = HB_STATE_IDLE;

    h->pause_lock = hb_lock_init();

    /* libavcodec */
    avcodec_init();
    avcodec_register_all();

    /* Start library thread */
    hb_log( "hb_init: starting libhb thread" );
    h->die         = 0;
    h->main_thread = hb_thread_init( "libhb", thread_func, h,
                                     HB_NORMAL_PRIORITY );

    hb_register( &hb_sync );
	hb_register( &hb_decmpeg2 );
	hb_register( &hb_decsub );
	hb_register( &hb_render );
	hb_register( &hb_encavcodec );
	hb_register( &hb_encxvid );
	hb_register( &hb_encx264 );
    hb_register( &hb_enctheora );
	hb_register( &hb_deca52 );
	hb_register( &hb_decdca );
	hb_register( &hb_decavcodec );
	hb_register( &hb_decavcodecv );
	hb_register( &hb_decavcodecvi );
	hb_register( &hb_decavcodecai );
	hb_register( &hb_declpcm );
	hb_register( &hb_encfaac );
	hb_register( &hb_enclame );
	hb_register( &hb_encvorbis );

	return h;
}


/**
 * Returns current version of libhb.
 * @param h Handle to hb_handle_t.
 * @return character array of version number.
 */
char * hb_get_version( hb_handle_t * h )
{
    return HB_VERSION;
}

/**
 * Returns current build of libhb.
 * @param h Handle to hb_handle_t.
 * @return character array of build number.
 */
int hb_get_build( hb_handle_t * h )
{
    return HB_BUILD;
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
 * Sets the cpu count to the desired value.
 * @param h Handle to hb_handle_t
 * @param cpu_count Number of CPUs to use.
 */
void hb_set_cpu_count( hb_handle_t * h, int cpu_count )
{
    cpu_count    = MAX( 1, cpu_count );
    cpu_count    = MIN( cpu_count, 8 );
    h->cpu_count = cpu_count;
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
              int preview_count, int store_previews )
{
    hb_title_t * title;

    /* Clean up from previous scan */
    while( ( title = hb_list_item( h->list_title, 0 ) ) )
    {
        hb_list_rem( h->list_title, title );
        hb_title_close( &title );
    }

    hb_log( "hb_scan: path=%s, title_index=%d", path, title_index );
    h->scan_thread = hb_scan_init( h, path, title_index, h->list_title,
                                   preview_count, store_previews );
}

/**
 * Returns the list of titles found.
 * @param h Handle to hb_handle_t
 * @return Handle to hb_list_t of the title list.
 */
hb_list_t * hb_get_titles( hb_handle_t * h )
{
    return h->list_title;
}

/**
 * Create preview image of desired title a index of picture.
 * @param h Handle to hb_handle_t.
 * @param title Handle to hb_title_t of desired title.
 * @param picture Index in title.
 * @param buffer Handle to buufer were inage will be drawn.
 */
void hb_get_preview( hb_handle_t * h, hb_title_t * title, int picture,
                     uint8_t * buffer )
{
    hb_job_t           * job = title->job;
    char                 filename[1024];
    FILE               * file;
    uint8_t            * buf1, * buf2, * buf3, * buf4, * pen;
    uint32_t           * p32, swsflags;
    AVPicture            pic_in, pic_preview, pic_deint, pic_crop, pic_scale;
    struct SwsContext  * context;
    int                  i;
    int                  rgb_width = ((job->width + 7) >> 3) << 3;
    int                  preview_size;

    swsflags = SWS_LANCZOS;
#ifndef __x86_64__
    swsflags |= SWS_ACCURATE_RND;
#endif  /* __x86_64__ */

    buf1 = av_malloc( avpicture_get_size( PIX_FMT_YUV420P, title->width, title->height ) );
    buf2 = av_malloc( avpicture_get_size( PIX_FMT_YUV420P, title->width, title->height ) );
    buf3 = av_malloc( avpicture_get_size( PIX_FMT_YUV420P, job->width, job->height ) );
    buf4 = av_malloc( avpicture_get_size( PIX_FMT_RGBA32, rgb_width, job->height ) );
    avpicture_fill( &pic_in, buf1, PIX_FMT_YUV420P,
                    title->width, title->height );
    avpicture_fill( &pic_deint, buf2, PIX_FMT_YUV420P,
                    title->width, title->height );
    avpicture_fill( &pic_scale, buf3, PIX_FMT_YUV420P,
                    job->width, job->height );
    avpicture_fill( &pic_preview, buf4, PIX_FMT_RGBA32,
                    rgb_width, job->height );

    // Allocate the AVPicture frames and fill in

    memset( filename, 0, 1024 );

    hb_get_tempory_filename( h, filename, "%x%d",
                             (intptr_t) title, picture );

    file = fopen( filename, "r" );
    if( !file )
    {
        hb_log( "hb_get_preview: fopen failed" );
        return;
    }

    fread( buf1, avpicture_get_size( PIX_FMT_YUV420P, title->width, title->height), 1, file );
    fclose( file );

    if( job->deinterlace )
    {
        // Deinterlace and crop
        avpicture_deinterlace( &pic_deint, &pic_in, PIX_FMT_YUV420P, title->width, title->height );
        av_picture_crop( &pic_crop, &pic_deint, PIX_FMT_YUV420P, job->crop[0], job->crop[2] );
    }
    else
    {
        // Crop
        av_picture_crop( &pic_crop, &pic_in, PIX_FMT_YUV420P, job->crop[0], job->crop[2] );
    }

    // Get scaling context
    context = sws_getContext(title->width  - (job->crop[2] + job->crop[3]),
                             title->height - (job->crop[0] + job->crop[1]),
                             PIX_FMT_YUV420P,
                             job->width, job->height, PIX_FMT_YUV420P,
                             swsflags, NULL, NULL, NULL);

    // Scale
    sws_scale(context,
              pic_crop.data, pic_crop.linesize,
              0, title->height - (job->crop[0] + job->crop[1]),
              pic_scale.data, pic_scale.linesize);

    // Free context
    sws_freeContext( context );

    // Get preview context
    context = sws_getContext(rgb_width, job->height, PIX_FMT_YUV420P,
                              rgb_width, job->height, PIX_FMT_RGBA32,
                              swsflags, NULL, NULL, NULL);

    // Create preview
    sws_scale(context,
              pic_scale.data, pic_scale.linesize,
              0, job->height,
              pic_preview.data, pic_preview.linesize);

    // Free context
    sws_freeContext( context );

    /* Gray background */
    p32 = (uint32_t *) buffer;
    for( i = 0; i < ( title->width + 2 ) * ( title->height + 2 ); i++ )
    {
        p32[i] = 0xFF808080;
    }

    /* Draw the picture, centered, and draw the cropping zone */
    preview_size = pic_preview.linesize[0];
    pen = buffer + ( title->height - job->height ) *
        ( title->width + 2 ) * 2 + ( title->width - job->width ) * 2;
    memset( pen, 0xFF, 4 * ( job->width + 2 ) );
    pen += 4 * ( title->width + 2 );
    for( i = 0; i < job->height; i++ )
    {
        uint8_t * nextLine;
        nextLine = pen + 4 * ( title->width + 2 );
        memset( pen, 0xFF, 4 );
        pen += 4;
        memcpy( pen, buf4 + preview_size * i, 4 * job->width );
        pen += 4 * job->width;
        memset( pen, 0xFF, 4 );
        pen = nextLine;
    }
    memset( pen, 0xFF, 4 * ( job->width + 2 ) );

    // Clean up
    avpicture_free( &pic_preview );
    avpicture_free( &pic_scale );
    avpicture_free( &pic_deint );
    avpicture_free( &pic_in );
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
int hb_detect_comb( hb_buffer_t * buf, int width, int height, int color_equal, int color_diff, int threshold, int prog_equal, int prog_diff, int prog_threshold )
{
    int j, k, n, off, cc_1, cc_2, cc[3], flag[3] ;
    uint16_t s1, s2, s3, s4;
    cc_1 = 0; cc_2 = 0;

    int offset = 0;
    
    if ( buf->flags & 16 )
    {
        /* Frame is progressive, be more discerning. */
        color_diff = prog_diff;
        color_equal = prog_equal;
        threshold = prog_threshold;
    }

    /* One pas for Y, one pass for Cb, one pass for Cr */    
    for( k = 0; k < 3; k++ )
    {
        if( k == 1 )
        {
            /* Y has already been checked, now offset by Y's dimensions
               and divide all the other values by 2, since Cr and Cb
               are half-size compared to Y.                               */
            offset = width * height;
            width >>= 1;
            height >>= 1;
        }
        else if ( k == 2 )
        {
            /* Y and Cb are done, so the offset needs to be bumped
               so it's width*height + (width / 2) * (height / 2)  */
            offset *= 5/4;
        }

        for( j = 0; j < width; ++j )
        {
            off = 0;

            for( n = 0; n < ( height - 4 ); n = n + 2 )
            {
                /* Look at groups of 4 sequential horizontal lines */
                s1 = ( ( buf->data + offset )[ off + j             ] & 0xff );
                s2 = ( ( buf->data + offset )[ off + j + width     ] & 0xff );
                s3 = ( ( buf->data + offset )[ off + j + 2 * width ] & 0xff );
                s4 = ( ( buf->data + offset )[ off + j + 3 * width ] & 0xff );

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
                off += 2 * width;
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
            hb_log("Average %i combed (Threshold %i) %i/%i/%i | PTS: %lld (%fs) %s", average_cc, threshold, cc[0], cc[1], cc[2], buf->start, (float)buf->start / 90000, (buf->flags & 16) ? "Film" : "Video" );
#endif
        return 1;
    }

#if 0
    hb_log("SKIPPED Average %i combed (Threshold %i) %i/%i/%i | PTS: %lld (%fs) %s", average_cc, threshold, cc[0], cc[1], cc[2], buf->start, (float)buf->start / 90000, (buf->flags & 16) ? "Film" : "Video" );
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
 @ param output_par_height Pointer to returned pixel height
 */
void hb_set_anamorphic_size( hb_job_t * job,
        int *output_width, int *output_height,
        int *output_par_width, int *output_par_height )
{
    /* "Loose" anamorphic.
        - Uses mod16-compliant dimensions,
        - Allows users to set the width
        - Handles ITU pixel aspects
    */

    /* Set up some variables to make the math easier to follow. */
    hb_title_t * title = job->title;
    int cropped_width = title->width - job->crop[2] - job->crop[3] ;
    int cropped_height = title->height - job->crop[0] - job->crop[1] ;
    double storage_aspect = (double)cropped_width / (double)cropped_height;
    int width = job->width;
    int height; // Gets set later, ignore user job->height value
    int mod = job->modulus;
    double aspect = title->aspect;

    /* Gotta handle bounding dimensions differently
       than for non-anamorphic encodes:
       If the width is too big, just reset it with no rescaling.
       Instead of using the aspect-scaled job height,
       we need to see if the job width divided by the storage aspect
       is bigger than the max. If so, set it to the max (this is sloppy).
       If not, set job height to job width divided by storage aspect.
    */

    if ( job->maxWidth && (job->maxWidth < job->width) )
        width = job->maxWidth;

    height = (double)width / storage_aspect;
    if ( job->maxHeight && (job->maxHeight < height) )
        height = job->maxHeight;

    /* In case the user specified a modulus, use it */
    if (job->modulus)
        mod = job->modulus;
    else
        mod = 16;

    /* Time to get picture dimensions that divide cleanly.*/
    width  = MULTIPLE_MOD( width, mod);
    height = MULTIPLE_MOD( height, mod);

    /* Verify these new dimensions don't violate max height and width settings */
    if ( job->maxWidth && (job->maxWidth < job->width) )
        width = job->maxWidth;
    if ( job->maxHeight && (job->maxHeight < height) )
        height = job->maxHeight;
    
    int pixel_aspect_width = job->pixel_aspect_width;
    int pixel_aspect_height = job->pixel_aspect_height;
    
    /* If a source was really 704*480 and hard matted with cropping
       to 720*480, replace the PAR values with the ITU broadcast ones. */
    if (title->width == 720 && cropped_width <= 706)
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

    /* Figure out what dimensions the source would display at. */
    int source_display_width = cropped_width * (double)pixel_aspect_width /
                               (double)pixel_aspect_height ;

    /* The film AR is the source's display width / cropped source height.
       The output display width is the output height * film AR.
       The output PAR is the output display width / output storage width. */
    pixel_aspect_width = height * source_display_width / cropped_height;
    pixel_aspect_height = width;

    /* Pass the results back to the caller */
    *output_width = width;
    *output_height = height;

    /* While x264 is smart enough to reduce fractions on its own, libavcodec
       needs some help with the math, so lose superfluous factors.            */
    hb_reduce( output_par_width, output_par_height,
               pixel_aspect_width, pixel_aspect_height );
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
    hb_title_t    * title,    * title_copy;
    hb_chapter_t  * chapter,  * chapter_copy;
    hb_audio_t    * audio;
    hb_subtitle_t * subtitle, * subtitle_copy;
    int             i;
    char            audio_lang[4];

    /* Copy the title */
    title      = job->title;
    title_copy = malloc( sizeof( hb_title_t ) );
    memcpy( title_copy, title, sizeof( hb_title_t ) );

    title_copy->list_chapter = hb_list_init();
    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        chapter      = hb_list_item( title->list_chapter, i );
        chapter_copy = malloc( sizeof( hb_chapter_t ) );
        memcpy( chapter_copy, chapter, sizeof( hb_chapter_t ) );
        hb_list_add( title_copy->list_chapter, chapter_copy );
    }

    /*
     * Copy the metadata
     */
    if( title->metadata )
    {
        title_copy->metadata = malloc( sizeof( hb_metadata_t ) );
        
        if( title_copy->metadata ) 
        {
            memcpy( title_copy->metadata, title->metadata, sizeof( hb_metadata_t ) );

            /*
             * Need to copy the artwork seperatly (TODO).
             */
            if( title->metadata->coverart )
            {
                title_copy->metadata->coverart = malloc( title->metadata->coverart_size );
                if( title_copy->metadata->coverart )
                {
                    memcpy( title_copy->metadata->coverart, title->metadata->coverart,
                            title->metadata->coverart_size );
                } else {
                    title_copy->metadata->coverart_size = 0; 
                }
            }
        }
    }

    /* Copy the audio track(s) we want */
    title_copy->list_audio = hb_list_init();

    for( i = 0; i < hb_list_count(job->list_audio); i++ )
    {
        if( ( audio = hb_list_item( job->list_audio, i ) ) )
        {
            hb_list_add( title_copy->list_audio, hb_audio_copy(audio) );
        }
    }

    title_copy->list_subtitle = hb_list_init();

    /*
     * The following code is confusing, there are three ways in which
     * we select subtitles and it depends on whether this is single or
     * two pass mode.
     *
     * subtitle_scan may be enabled, in which case the first pass
     * scans all subtitles of that language. The second pass does not
     * select any because they are set at the end of the first pass.
     *
     * native_language may have a preferred language, in which case we
     * may be switching the language we want for the subtitles in the
     * first pass of a single pass, or the second pass of a two pass.
     *
     * We may have manually selected a subtitle, in which case that is
     * selected in the first pass of a single pass, or the second of a
     * two pass.
     */
    memset( audio_lang, 0, sizeof( audio_lang ) );

    if ( job->indepth_scan || job->native_language ) {

        /*
         * Find the first audio language that is being encoded
         */
        for( i = 0; i < hb_list_count(job->list_audio); i++ )
        {
            if( ( audio = hb_list_item( job->list_audio, i ) ) )
            {
                strncpy(audio_lang, audio->config.lang.iso639_2, sizeof(audio_lang));
                break;
            }
        }

        /*
         * In all cases switch the language if we need to to our native
         * language.
         */
        if( job->native_language )
        {
            if( strncasecmp( job->native_language, audio_lang,
                             sizeof( audio_lang ) ) != 0 )
            {

                if( job->pass != 2 )
                {
                    hb_log( "Enabled subtitles in native language '%s', audio is in '%s'",
                            job->native_language, audio_lang);
                }
                /*
                 * The main audio track is not in our native language, so switch
                 * the subtitles to use our native language instead.
                 */
                strncpy( audio_lang, job->native_language, sizeof( audio_lang ) );
            } else {
                /*
                 * native language is irrelevent, free it.
                 */
                free( job->native_language );
                job->native_language = NULL;
            }
        }
    }

    /*
     * If doing a subtitle scan then add all the matching subtitles for this
     * language.
     */
    if ( job->indepth_scan )
    {
        for( i=0; i < hb_list_count( title->list_subtitle ); i++ )
        {
            subtitle = hb_list_item( title->list_subtitle, i );
            if( strcmp( subtitle->iso639_2, audio_lang ) == 0 )
            {
                /*
                 * Matched subtitle language with audio language, so
                 * add this to our list to scan.
                 *
                 * We will update the subtitle list on the second pass
                 * later after the first pass has completed.
                 */
                subtitle_copy = malloc( sizeof( hb_subtitle_t ) );
                memcpy( subtitle_copy, subtitle, sizeof( hb_subtitle_t ) );
                hb_list_add( title_copy->list_subtitle, subtitle_copy );
                if ( job->native_language ) {
                    /*
                     * With native language just select the
                     * first match in our langiage, not all of
                     * them. Subsequent ones are likely to be commentary
                     */
                    break;
                }
            }
        }
    } else {
        /*
         * Not doing a subtitle scan in this pass, but maybe we are in the
         * first pass?
         */
        if( job->select_subtitle )
        {
            /*
             * Don't add subtitles here, we'll add them via select_subtitle
             * at the end of the subtitle_scan.
             */
        } else {
            /*
             * Definitely not doing a subtitle scan.
             */
            if( job->pass != 1 && job->native_language )
            {
                /*
                 * We are not doing a subtitle scan but do want the
                 * native langauge subtitle selected, so select it
                 * for pass 0 or pass 2 of a two pass.
                 */
                for( i=0; i < hb_list_count( title->list_subtitle ); i++ )
                {
                    subtitle = hb_list_item( title->list_subtitle, i );
                    if( strcmp( subtitle->iso639_2, audio_lang ) == 0 )
                    {
                        /*
                         * Matched subtitle language with audio language, so
                         * add this to our list to scan.
                         */
                        subtitle_copy = malloc( sizeof( hb_subtitle_t ) );
                        memcpy( subtitle_copy, subtitle, sizeof( hb_subtitle_t ) );
                        hb_list_add( title_copy->list_subtitle, subtitle_copy );
                        break;
                    }
                }
            } else {
                /*
                 * Manually selected subtitle, in which case only
                 * bother adding them for pass 0 or pass 2 of a two
                 * pass.
                 */
                if( job->pass != 1 )
                {
                    if( ( subtitle = hb_list_item( title->list_subtitle, job->subtitle ) ) )
                    {
                        subtitle_copy = malloc( sizeof( hb_subtitle_t ) );
                        memcpy( subtitle_copy, subtitle, sizeof( hb_subtitle_t ) );
                        hb_list_add( title_copy->list_subtitle, subtitle_copy );
                    }
                }
            }
        }
    }

    /* Copy the job */
    job_copy        = calloc( sizeof( hb_job_t ), 1 );
    memcpy( job_copy, job, sizeof( hb_job_t ) );
    title_copy->job = job_copy;
    job_copy->title = title_copy;
    job_copy->list_audio = title_copy->list_audio;
    job_copy->file  = strdup( job->file );
    job_copy->h     = h;
    job_copy->pause = h->pause_lock;

    /* Copy the job filter list */
    if( job->filters )
    {
        int i;
        int filter_count = hb_list_count( job->filters );
        job_copy->filters = hb_list_init();
        for( i = 0; i < filter_count; i++ )
        {
            /*
             * Copy the filters, since the MacGui reuses the global filter objects
             * meaning that queued up jobs overwrite the previous filter settings.
             * In reality, settings is probably the only field that needs duplicating
             * since it's the only value that is ever changed. But name is duplicated
             * as well for completeness. Not copying private_data since it gets
             * created for each job in renderInit.
             */
            hb_filter_object_t * filter = hb_list_item( job->filters, i );
            hb_filter_object_t * filter_copy = malloc( sizeof( hb_filter_object_t ) );
            memcpy( filter_copy, filter, sizeof( hb_filter_object_t ) );
            if( filter->name )
                filter_copy->name = strdup( filter->name );
            if( filter->settings )
                filter_copy->settings = strdup( filter->settings );
            hb_list_add( job_copy->filters, filter_copy );
        }
    }

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
    h->work_thread = hb_work_init( h->jobs, h->cpu_count,
                                   &h->work_die, &h->work_error, &h->current_job );
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
 * Closes access to libhb by freeing the hb_handle_t handle ontained in hb_init_real.
 * @param _h Pointer to handle to hb_handle_t.
 */
void hb_close( hb_handle_t ** _h )
{
    hb_handle_t * h = *_h;
    hb_title_t * title;

    h->die = 1;
    hb_thread_close( &h->main_thread );

    while( ( title = hb_list_item( h->list_title, 0 ) ) )
    {
        hb_list_rem( h->list_title, title );
        if( title->job && title->job->filters )
        {
            hb_list_close( &title->job->filters );
        }
        free( title->job );
        hb_title_close( &title );
    }
    hb_list_close( &h->list_title );

    hb_list_close( &h->jobs );
    hb_lock_close( &h->state_lock );
    hb_lock_close( &h->pause_lock );
    free( h );
    *_h = NULL;

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
    DIR * dir;
    struct dirent * entry;

    h->pid = getpid();

    /* Create folder for temporary files */
    memset( dirname, 0, 1024 );
    hb_get_tempory_directory( h, dirname );

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

            hb_log( "libhb: scan thread found %d valid title(s)",
                    hb_list_count( h->list_title ) );
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

    if( h->work_thread )
    {
        hb_stop( h );
        hb_thread_close( &h->work_thread );
    }

    /* Remove temp folder */
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
 * Returns the PID.
 * @param h Handle to hb_handle_t
 */
int hb_get_pid( hb_handle_t * h )
{
    return h->pid;
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
    if( h->state.state == HB_STATE_WORKING )
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
