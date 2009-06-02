/* $Id: test.c,v 1.82 2005/11/19 08:25:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#include "hb.h"
#include "parsecsv.h"

#ifdef __APPLE_CC__
#import <CoreServices/CoreServices.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#endif

/* Options */
static int    debug       = HB_DEBUG_NONE;
static int    update      = 0;
static int    dvdnav      = 0;
static char * input       = NULL;
static char * output      = NULL;
static char * format      = NULL;
static int    titleindex  = 1;
static int    longest_title = 0;
static int    subtitle_scan = 0;
static int    subtitle_force = 0;
static char * native_language = NULL;
static int    twoPass     = 0;
static int    deinterlace           = 0;
static char * deinterlace_opt       = 0;
static int    deblock               = 0;
static char * deblock_opt           = 0;
static int    denoise               = 0;
static char * denoise_opt           = 0;
static int    detelecine            = 0;
static char * detelecine_opt        = 0;
static int    decomb                = 0;
static char * decomb_opt            = 0;
static int    grayscale   = 0;
static int    vcodec      = HB_VCODEC_FFMPEG;
static int    h264_13     = 0;
static int    h264_30     = 0;
static hb_list_t * audios = NULL;
static hb_audio_config_t * audio = NULL;
static int    num_audio_tracks = 0;
static char * mixdowns    = NULL;
static char * dynamic_range_compression = NULL;
static char * atracks     = NULL;
static char * arates      = NULL;
static char * abitrates   = NULL;
static char * acodecs     = NULL;
static char * anames      = NULL;
static int    default_acodec = HB_ACODEC_FAAC;
static int    default_arate = 48000;
static int    default_abitrate = 160;
static int    sub         = 0;
static int    width       = 0;
static int    height      = 0;
static int    crop[4]     = { -1,-1,-1,-1 };
static int    cpu         = 0;
static int    vrate       = 0;
static float  vquality    = -1.0;
static int    vbitrate    = 0;
static int    size        = 0;
static int    mux         = 0;
static int    pixelratio  = 0;
static int    loosePixelratio = 0;
static int    modulus       = 0;
static int    par_height    = 0;
static int    par_width     = 0;
static int    angle = 0;
static int    chapter_start = 0;
static int    chapter_end   = 0;
static int    chapter_markers = 0;
static char * marker_file   = NULL;
static int	  crf			= 1;
static char	  *x264opts		= NULL;
static char	  *x264opts2 	= NULL;
static int	  maxHeight		= 0;
static int	  maxWidth		= 0;
static int    turbo_opts_enabled = 0;
static char * turbo_opts = "ref=1:subme=1:me=dia:analyse=none:trellis=0:no-fast-pskip=0:8x8dct=0:weightb=0";
static int    largeFileSize = 0;
static int    preset        = 0;
static char * preset_name   = 0;
static int    cfr           = 0;
static int    mp4_optimize  = 0;
static int    ipod_atom     = 0;
static int    color_matrix  = 0;
static int    preview_count = 10;
static int    store_previews = 0;
static int    start_at_preview = 0;
static int64_t stop_at_pts    = 0;
static int    stop_at_frame = 0;
static char * stop_at_string = NULL;
static char * stop_at_token = NULL;

/* Exit cleanly on Ctrl-C */
static volatile int die = 0;
static void SigHandler( int );

/* Utils */
static void ShowCommands();
static void ShowHelp();
static void ShowPresets();

static int  ParseOptions( int argc, char ** argv );
static int  CheckOptions( int argc, char ** argv );
static int  HandleEvents( hb_handle_t * h );

static int get_acodec_for_string( char *codec );
static int is_sample_rate_valid(int rate);

#ifdef __APPLE_CC__
static char* bsd_name_for_path(char *path);
static int device_is_dvd(char *device);
static io_service_t get_iokit_service( char *device );
static int is_dvd_service( io_service_t service );
static is_whole_media_service( io_service_t service );
#endif

/* Only print the "Muxing..." message once */
static int show_mux_warning = 1;

/****************************************************************************
 * hb_error_handler
 *
 * When using the CLI just display using hb_log as we always did in the past
 * make sure that we prefix with a nice ERROR message to catch peoples eyes.
 ****************************************************************************/
static void hb_cli_error_handler ( const char *errmsg )
{
    fprintf( stderr, "ERROR: %s\n", errmsg );
}

int main( int argc, char ** argv )
{
    hb_handle_t * h;
    int           build;
    char        * version;

/* win32 _IOLBF (line-buffering) is the same as _IOFBF (full-buffering).
 * force it to unbuffered otherwise informative output is not easily parsed.
 */
#if defined( _WIN32 ) || defined( __MINGW32__ )
    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );
#endif

    audios = hb_list_init();

    /* Parse command line */
    if( ParseOptions( argc, argv ) ||
        CheckOptions( argc, argv ) )
    {
        return 1;
    }

#ifdef PTW32_STATIC_LIB
    pthread_win32_process_attach_np();
    pthread_win32_thread_attach_np();
#endif

    /* Register our error handler */
    hb_register_error_handler(&hb_cli_error_handler);

    /* Init libhb */
    h = hb_init( debug, update );
    hb_dvd_set_dvdnav( dvdnav );

    /* Show version */
    fprintf( stderr, "%s - %s - %s\n",
             HB_PROJECT_TITLE, HB_PROJECT_BUILD_TITLE, HB_PROJECT_URL_WEBSITE );

    /* Check for update */
    if( update )
    {
        if( ( build = hb_check_update( h, &version ) ) > -1 )
        {
            fprintf( stderr, "You are using an old version of "
                     "HandBrake.\nLatest is %s (build %d).\n", version,
                     build );
        }
        else
        {
            fprintf( stderr, "Your version of HandBrake is up to "
                     "date.\n" );
        }
        hb_close( &h );
        return 0;
    }

    /* Geeky */
    fprintf( stderr, "%d CPU%s detected\n", hb_get_cpu_count(),
             hb_get_cpu_count( h ) > 1 ? "s" : "" );
    if( cpu )
    {
        fprintf( stderr, "Forcing %d CPU%s\n", cpu,
                 cpu > 1 ? "s" : "" );
        hb_set_cpu_count( h, cpu );
    }

    /* Exit ASAP on Ctrl-C */
    signal( SIGINT, SigHandler );

    /* Feed libhb with a DVD to scan */
    fprintf( stderr, "Opening %s...\n", input );

    if (longest_title) {
        /*
         * We need to scan for all the titles in order to find the longest
         */
        titleindex = 0;
    }

    hb_scan( h, input, titleindex, preview_count, store_previews );

    /* Wait... */
    while( !die )
    {
#if !defined(SYS_BEOS) && !defined(__MINGW32__)
        fd_set         fds;
        struct timeval tv;
        int            ret;
        char           buf[257];

        tv.tv_sec  = 0;
        tv.tv_usec = 100000;

        FD_ZERO( &fds );
        FD_SET( STDIN_FILENO, &fds );
        ret = select( STDIN_FILENO + 1, &fds, NULL, NULL, &tv );

        if( ret > 0 )
        {
            int size = 0;

            while( size < 256 &&
                   read( STDIN_FILENO, &buf[size], 1 ) > 0 )
            {
                if( buf[size] == '\n' )
                {
                    break;
                }
                size++;
            }

            if( size >= 256 || buf[size] == '\n' )
            {
                switch( buf[0] )
                {
                    case 'q':
                        fprintf( stdout, "\nEncoding Quit by user command\n" );
                        die = 1;
                        break;
                    case 'p':
                        fprintf( stdout, "\nEncoding Paused by user command, 'r' to resume\n" );
                        hb_pause( h );
                        break;
                    case 'r':
                        hb_resume( h );
                        break;
                    case 'h':
                        ShowCommands();
                        break;
                }
            }
        }
        hb_snooze( 200 );
#else
        hb_snooze( 200 );
#endif

        HandleEvents( h );
    }

    /* Clean up */
    hb_close( &h );
    if( input )  free( input );
    if( output ) free( output );
    if( format ) free( format );
    if( audios )
    {
        while( ( audio = hb_list_item( audios, 0 ) ) )
        {
            hb_list_rem( audios, audio );
            if( audio->out.name )
            {
                free( audio->out.name );
            }
            free( audio );
        }
        hb_list_close( &audios );
    }
    if( mixdowns ) free( mixdowns );
    if( dynamic_range_compression ) free( dynamic_range_compression );
    if( atracks ) free( atracks );
    if( arates ) free( arates );
    if( abitrates ) free( abitrates );
    if( acodecs ) free( acodecs );
    if( anames ) free( anames );
    if (native_language ) free (native_language );
	if( x264opts ) free (x264opts );
	if( x264opts2 ) free (x264opts2 );
    if (preset_name) free (preset_name);
    if( stop_at_string ) free( stop_at_string );

    fprintf( stderr, "HandBrake has exited.\n" );

#ifdef PTW32_STATIC_LIB
    pthread_win32_thread_detach_np();
    pthread_win32_process_detach_np();
#endif

    return 0;
}

static void ShowCommands()
{
    fprintf( stdout, "\nCommands:\n" );
    fprintf( stdout, " [h]elp    Show this message\n" );
    fprintf( stdout, " [q]uit    Exit HandBrakeCLI\n" );
    fprintf( stdout, " [p]ause   Pause encoding\n" );
    fprintf( stdout, " [r]esume  Resume encoding\n" );
}

static void PrintTitleInfo( hb_title_t * title )
{
    hb_chapter_t  * chapter;
    hb_audio_config_t    * audio;
    hb_subtitle_t * subtitle;
    int i;

    fprintf( stderr, "+ title %d:\n", title->index );
    fprintf( stderr, "  + vts %d, ttn %d, cells %d->%d (%d blocks)\n",
             title->vts, title->ttn, title->cell_start, title->cell_end,
             title->block_count );
    if (dvdnav)
        fprintf( stderr, "  + angle(s) %d\n", title->angle_count );
    fprintf( stderr, "  + duration: %02d:%02d:%02d\n",
             title->hours, title->minutes, title->seconds );
    fprintf( stderr, "  + size: %dx%d, aspect: %.2f, %.3f fps\n",
             title->width, title->height,
             (float) title->aspect,
             (float) title->rate / title->rate_base );
    fprintf( stderr, "  + autocrop: %d/%d/%d/%d\n", title->crop[0],
             title->crop[1], title->crop[2], title->crop[3] );
    fprintf( stderr, "  + chapters:\n" );
    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        chapter = hb_list_item( title->list_chapter, i );
        fprintf( stderr, "    + %d: cells %d->%d, %d blocks, duration "
                 "%02d:%02d:%02d\n", chapter->index,
                 chapter->cell_start, chapter->cell_end,
                 chapter->block_count, chapter->hours, chapter->minutes,
                 chapter->seconds );
    }
    fprintf( stderr, "  + audio tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_audio_config_item( title->list_audio, i );
        if( ( audio->in.codec == HB_ACODEC_AC3 ) || ( audio->in.codec == HB_ACODEC_DCA) )
        {
            fprintf( stderr, "    + %d, %s, %dHz, %dbps\n", i + 1,
                     audio->lang.description, audio->in.samplerate, audio->in.bitrate );
        }
        else
        {
            fprintf( stderr, "    + %d, %s\n", i + 1, audio->lang.description );
        }
    }
    fprintf( stderr, "  + subtitle tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        subtitle = hb_list_item( title->list_subtitle, i );
        fprintf( stderr, "    + %d, %s (iso639-2: %s)\n", i + 1, subtitle->lang,
            subtitle->iso639_2);
    }

    if(title->detected_interlacing)
    {
        /* Interlacing was found in half or more of the preview frames */
        fprintf( stderr, "  + combing detected, may be interlaced or telecined\n");
    }

}

static int HandleEvents( hb_handle_t * h )
{
    hb_state_t s;
    int tmp_num_audio_tracks;

    hb_get_state( h, &s );
    switch( s.state )
    {
        case HB_STATE_IDLE:
            /* Nothing to do */
            break;

#define p s.param.scanning
        case HB_STATE_SCANNING:
            /* Show what title is currently being scanned */
            fprintf( stderr, "Scanning title %d", p.title_cur );
            if( !titleindex )
                fprintf( stderr, " of %d", p.title_count );
            fprintf( stderr, "...\n" );
            break;
#undef p

        case HB_STATE_SCANDONE:
        {
            hb_list_t  * list;
            hb_title_t * title;
            hb_job_t   * job;
            int i;

            /* Audio argument string parsing variables */
            int acodec = 0;
            int abitrate = 0;
            int arate = 0;
            int mixdown = HB_AMIXDOWN_DOLBYPLII;
            double d_r_c = 0;
            /* Audio argument string parsing variables */

            list = hb_get_titles( h );

            if( !hb_list_count( list ) )
            {
                /* No valid title, stop right there */
                fprintf( stderr, "No title found.\n" );
                die = 1;
                break;
            }
	    if( longest_title )
	    {
                int i;
                int longest_title_idx=0;
                int longest_title_pos=-1;
                int longest_title_time=0;
                int title_time;

                fprintf( stderr, "Searching for longest title...\n" );

                for( i = 0; i < hb_list_count( list ); i++ )
                {
                    title = hb_list_item( list, i );
                    title_time = (title->hours*60*60 ) + (title->minutes *60) + (title->seconds);
                    fprintf( stderr, " + Title (%d) index %d has length %dsec\n",
                             i, title->index, title_time );
                    if( longest_title_time < title_time )
                    {
                        longest_title_time = title_time;
                        longest_title_pos = i;
                        longest_title_idx = title->index;
                    }
                }
                if( longest_title_pos == -1 )
                {
                    fprintf( stderr, "No longest title found.\n" );
                    die = 1;
                    break;
                }
                titleindex = longest_title_idx;
                fprintf( stderr, "Found longest title, setting title to %d\n",
                         longest_title_idx);

                title = hb_list_item( list, longest_title_pos);
            } else {
                title = hb_list_item( list, 0 );
            }

            if( !titleindex )
            {
                /* Scan-only mode, print infos and exit */
                int i;
                for( i = 0; i < hb_list_count( list ); i++ )
                {
                    title = hb_list_item( list, i );
                    PrintTitleInfo( title );
                }
                die = 1;
                break;
            }

            /* Set job settings */
            job   = title->job;

            PrintTitleInfo( title );

            if( chapter_start && chapter_end && !stop_at_pts && !start_at_preview && !stop_at_frame )
            {
                job->chapter_start = MAX( job->chapter_start,
                                          chapter_start );
                job->chapter_end   = MIN( job->chapter_end,
                                          chapter_end );
                job->chapter_end   = MAX( job->chapter_start,
                                          job->chapter_end );
            }

            if ( angle )
            {
                job->angle = angle;
            }

            if (preset)
            {
                fprintf( stderr, "+ Using preset: %s", preset_name);

                if (!strcmp(preset_name, "Universal"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vquality = 0.589999973773956;
                    job->crf = 1;
                    if( !atracks )
                    {
                        atracks = strdup("1,1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160,auto");
                    }
                    if( !arates )
                    {
                        arates = strdup("48,Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac,ac3");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2,auto");
                    }
                    maxWidth = 720;
                    if( !x264opts )
                    {
                        x264opts = strdup("level=30:cabac=0:ref=3:mixed-refs=1:analyse=all:me=umh:no-fast-pskip=1");
                    }
                    pixelratio = 2;
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "iPod"))
                {
                    mux = HB_MUX_MP4;
                    job->ipod_atom = 1;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 700;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("48");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    maxWidth = 320;
                    if( !x264opts )
                    {
                        x264opts = strdup("level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1");
                    }
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "iPhone & iPod Touch"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vquality = 0.589999973773956;
                    job->crf = 1;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("128");
                    }
                    if( !arates )
                    {
                        arates = strdup("48");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    maxWidth = 480;
                    if( !x264opts )
                    {
                        x264opts = strdup("level=30:cabac=0:ref=2:mixed-refs:analyse=all:me=umh:no-fast-pskip=1");
                    }
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "AppleTV"))
                {
                    mux = HB_MUX_MP4;
                    job->largeFileSize = 1;
                    vcodec = HB_VCODEC_X264;
                    job->vquality = 0.589999973773956;
                    job->crf = 1;
                    if( !atracks )
                    {
                        atracks = strdup("1,1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160,auto");
                    }
                    if( !arates )
                    {
                        arates = strdup("48,Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac,ac3");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2,auto");
                    }
                    maxWidth = 960;
                    if( !x264opts )
                    {
                        x264opts = strdup("level=30:cabac=0:ref=3:mixed-refs=1:bframes=6:weightb=1:direct=auto:no-fast-pskip=1:me=umh:subq=7:analyse=all");
                    }
                    pixelratio = 2;
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "QuickTime"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1800;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    if( !x264opts )
                    {
                        x264opts = strdup("ref=3:mixed-refs:bframes=3:weightb:direct=auto:me=umh:subme=7:analyse=all:8x8dct:trellis=1:no-fast-pskip=1:psy-rd=1,1");
                    }
                    pixelratio = 1;
                    job->chapter_markers = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "AppleTV Legacy"))
                {
                    mux = HB_MUX_MP4;
                    job->largeFileSize = 1;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 2500;
                    if( !atracks )
                    {
                        atracks = strdup("1,1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160,auto");
                    }
                    if( !arates )
                    {
                        arates = strdup("48,Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac,ac3");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2,auto");
                    }
                    if( !x264opts )
                    {
                        x264opts = strdup("bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:trellis=1:cabac=0");
                    }
                    pixelratio = 1;
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "iPhone Legacy"))
                {
                    mux = HB_MUX_MP4;
                    job->ipod_atom = 1;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 960;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("128");
                    }
                    if( !arates )
                    {
                        arates = strdup("48");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    maxWidth = 480;
                    if( !x264opts )
                    {
                        x264opts = strdup("level=30:cabac=0:ref=1:analyse=all:me=umh:no-fast-pskip=1:trellis=1");
                    }
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "iPod Legacy"))
                {
                    mux = HB_MUX_MP4;
                    job->ipod_atom = 1;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1500;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("48");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    maxWidth = 640;
                    if( !x264opts )
                    {
                        x264opts = strdup("level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1");
                    }
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "Normal"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1500;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    if( !x264opts )
                    {
                        x264opts = strdup("ref=2:bframes=2:me=umh");
                    }
                    pixelratio = 1;
                    job->chapter_markers = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "Classic"))
                {
                    mux = HB_MUX_MP4;
                    job->vbitrate = 1000;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                }

                if (!strcmp(preset_name, "Animation"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1000;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    if( !x264opts )
                    {
                        x264opts = strdup("ref=5:mixed-refs:bframes=6:weightb:direct=auto:b-pyramid:me=umh:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip:filter=2,2:psy-rd=1,1:subme=9");
                    }
                    detelecine = 1;
                    decomb = 1;
                    pixelratio = 1;
                    job->chapter_markers = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "Constant Quality Rate"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vquality = 0.600000023841858;
                    job->crf = 1;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("auto");
                    }
                    if( !arates )
                    {
                        arates = strdup("Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("ac3");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("auto");
                    }
                    if( !x264opts )
                    {
                        x264opts = strdup("ref=3:mixed-refs:bframes=3:b-pyramid:weightb:filter=-2,-1:trellis=1:analyse=all:8x8dct:me=umh:subme=9:psy-rd=1,1");
                    }
                    pixelratio = 1;
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "Film"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1800;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("auto");
                    }
                    if( !arates )
                    {
                        arates = strdup("Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("ac3");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("auto");
                    }
                    if( !x264opts )
                    {
                        x264opts = strdup("ref=3:mixed-refs:bframes=6:weightb:direct=auto:b-pyramid:me=umh:subme=9:analyse=all:8x8dct:trellis=1:no-fast-pskip:psy-rd=1,1");
                    }
                    pixelratio = 1;
                    job->chapter_markers = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "Television"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1300;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("Auto");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    if( !x264opts )
                    {
                        x264opts = strdup("ref=3:mixed-refs:bframes=6:weightb:direct=auto:b-pyramid:me=umh:subme=9:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip=1:psy-rd=1,1");
                    }
                    detelecine = 1;
                    decomb = 1;
                    pixelratio = 1;
                    job->chapter_markers = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "PSP"))
                {
                    mux = HB_MUX_MP4;
                    job->vbitrate = 1024;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("128");
                    }
                    if( !arates )
                    {
                        arates = strdup("48");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    maxWidth = 368;
                    maxHeight = 208;
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "PS3"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 2500;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("48");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    job->crop[0] = 0;
                    job->crop[1] = 0;
                    job->crop[2] = 0;
                    job->crop[4] - 0;
                    if( !x264opts )
                    {
                        x264opts = strdup("level=41:me=umh");
                    }
                    pixelratio = 1;
                }

                if (!strcmp(preset_name, "Xbox 360"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 2000;
                    if( !atracks )
                    {
                        atracks = strdup("1");
                    }
                    if( !abitrates )
                    {
                        abitrates = strdup("160");
                    }
                    if( !arates )
                    {
                        arates = strdup("48");
                    }
                    if( !acodecs )
                    {
                        acodecs = strdup("faac");
                    }
                    if( !mixdowns )
                    {
                        mixdowns = strdup("dpl2");
                    }
                    if( !x264opts )
                    {
                        x264opts = strdup("level=40:ref=2:mixed-refs:bframes=3:weightb:subme=9:direct=auto:b-pyramid:me=umh:analyse=all:no-fast-pskip:filter=-2,-1");
                    }
                    pixelratio = 1;
                    }
            }

			if ( chapter_markers )
			{
				job->chapter_markers = chapter_markers;

                if( marker_file != NULL )
                {
                    hb_csv_file_t * file = hb_open_csv_file( marker_file );
                    hb_csv_cell_t * cell;
                    int row = 0;
                    int chapter = 0;

                    fprintf( stderr, "Reading chapter markers from file %s\n", marker_file );

                    if( file == NULL )
                    {
                         fprintf( stderr, "Cannot open chapter marker file, using defaults\n" );
                    }
                    else
                    {
                        /* Parse the cells */
                        while( NULL != ( cell = hb_read_next_cell( file ) ) )
                        {
                            /* We have a chapter number */
                            if( cell->cell_col == 0 )
                            {
                                row = cell->cell_row;
                                chapter = atoi( cell->cell_text );
                            }

                            /* We have a chapter name */
                            if( cell->cell_col == 1 && row == cell->cell_row )
                            {
                                /* If we have a valid chapter, copy the string an terminate it */
                                if( chapter >= job->chapter_start && chapter <= job->chapter_end )
                                {
                                    hb_chapter_t * chapter_s;

                                    chapter_s = hb_list_item( job->title->list_chapter, chapter - 1);
                                    strncpy(chapter_s->title, cell->cell_text, 1023);
                                    chapter_s->title[1023] = '\0';
                                }
                            }


                            hb_dispose_cell( cell );
                        }

                        hb_close_csv_file( file );
                    }
                }
                else
                {
                    /* No marker file */

                    int number_of_chapters = hb_list_count(job->title->list_chapter);
                    int chapter;

                    for(chapter = 0; chapter <= number_of_chapters - 1 ; chapter++)
                    {
                        hb_chapter_t * chapter_s;
                        chapter_s = hb_list_item( job->title->list_chapter, chapter);
                        snprintf( chapter_s->title, 1023, "Chapter %i", chapter + 1 );
                        chapter_s->title[1023] = '\0';
                    }
                }
			}

            if( crop[0] >= 0 && crop[1] >= 0 &&
                crop[2] >= 0 && crop[3] >= 0 )
            {
                memcpy( job->crop, crop, 4 * sizeof( int ) );
            }

            job->deinterlace = deinterlace;
            job->grayscale   = grayscale;
            if (loosePixelratio)
            {
                job->anamorphic.mode = 2;
                if (modulus)
                {
                    job->anamorphic.modulus = modulus;
                }
                if( par_width && par_height )
                {
                    job->anamorphic.mode = 3;
                    job->anamorphic.par_width = par_width;
                    job->anamorphic.par_height = par_height;
                }
            }
            else
            {
                job->anamorphic.mode = pixelratio;
            }

            /* Add selected filters */
            job->filters = hb_list_init();
            if( detelecine )
            {
                hb_filter_detelecine.settings = detelecine_opt;
                hb_list_add( job->filters, &hb_filter_detelecine );
            }
            if( decomb )
            {
                hb_filter_decomb.settings = decomb_opt;
                hb_list_add( job->filters, &hb_filter_decomb );
            }
            if( deinterlace )
            {
                hb_filter_deinterlace.settings = deinterlace_opt;
                hb_list_add( job->filters, &hb_filter_deinterlace );
            }
            if( deblock )
            {
                hb_filter_deblock.settings = deblock_opt;
                hb_list_add( job->filters, &hb_filter_deblock );
            }
            if( denoise )
            {
                hb_filter_denoise.settings = denoise_opt;
                hb_list_add( job->filters, &hb_filter_denoise );
            }

            if( width && height )
            {
                job->width  = width;
                job->height = height;
            }
            else if( width )
            {
                job->width = width;
                hb_fix_aspect( job, HB_KEEP_WIDTH );
            }
            else if( height && !loosePixelratio)
            {
                job->height = height;
                hb_fix_aspect( job, HB_KEEP_HEIGHT );
            }
            else if( !width && !height && !pixelratio && !loosePixelratio )
            {
                hb_fix_aspect( job, HB_KEEP_WIDTH );
            }
            else if (!width && loosePixelratio)
            {
                /* Default to full width when one isn't specified for loose anamorphic */
                job->width = title->width - job->crop[2] - job->crop[3];
                /* The height will be thrown away in hb.c but calculate it anyway */
                hb_fix_aspect( job, HB_KEEP_WIDTH );
            }

            if( vquality >= 0.0 && ( ( vquality <= 1.0 ) || ( vcodec == HB_VCODEC_X264 ) || (vcodec == HB_VCODEC_FFMPEG) ) )
            {
                job->vquality = vquality;
                job->vbitrate = 0;
            }
            else if( vbitrate )
            {
                job->vquality = -1.0;
                job->vbitrate = vbitrate;
            }
            if( vcodec )
            {
                job->vcodec = vcodec;
            }
            if( h264_13 )
            {
                job->h264_level = 13;
            }
	        if( h264_30 )
	        {
	            job->h264_level = 30;
            }
            if( vrate )
            {
                job->cfr = cfr;
                job->vrate = 27000000;
                job->vrate_base = vrate;
            }
            else if ( cfr )
            {
                // cfr or pfr flag with no rate specified implies
                // use the title rate.
                job->cfr = cfr;
                job->vrate = title->rate;
                job->vrate_base = title->rate_base;
            }

            /* Grab audio tracks */
            if( atracks )
            {
                char * token = strtok(atracks, ",");
                if (token == NULL)
                    token = optarg;
                int track_start, track_end;
                while( token != NULL )
                {
                    audio = calloc(1, sizeof(*audio));
                    hb_audio_config_init(audio);
                    if (strlen(token) >= 3)
                    {
                        if (sscanf(token, "%d-%d", &track_start, &track_end) == 2)
                        {
                            int i;
                            for (i = track_start - 1; i < track_end; i++)
                            {
                                if (i != track_start - 1)
                                {
                                    audio = calloc(1, sizeof(*audio));
                                    hb_audio_config_init(audio);
                                }
                                audio->in.track = i;
                                audio->out.track = num_audio_tracks++;
                                hb_list_add(audios, audio);
                            }
                        }
                        else if( !strcasecmp(token, "none" ) )
                        {
                            audio->in.track = audio->out.track = -1;
                            audio->out.codec = 0;
                            hb_list_add(audios, audio);
                            break;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: Unable to parse audio input \"%s\", skipping.",
                                    token);
                            free(audio);
                        }
                    }
                    else
                    {
                        audio->in.track = atoi(token) - 1;
                        audio->out.track = num_audio_tracks++;
                        hb_list_add(audios, audio);
                    }
                    token = strtok(NULL, ",");
                }
            }

            /* Parse audio tracks */
            if( hb_list_count(audios) == 0 )
            {
                /* Create a new audio track with default settings */
                audio = calloc(1, sizeof(*audio));
                hb_audio_config_init(audio);
                /* Add it to our audios */
                hb_list_add(audios, audio);
            }

            tmp_num_audio_tracks = num_audio_tracks = hb_list_count(audios);
            for (i = 0; i < tmp_num_audio_tracks; i++)
            {
                audio = hb_list_item(audios, 0);
                if( (audio == NULL) || (audio->in.track == -1) ||
                    (audio->out.track == -1) || (audio->out.codec == 0) )
                {
                    num_audio_tracks--;
                }
                else
                {
                    if( hb_audio_add( job, audio ) == 0 )
                    {
                        fprintf(stderr, "ERROR: Invalid audio input track '%u', exiting.\n", 
                                audio->in.track + 1 );
                        num_audio_tracks--;
                        exit(3);
                    }
                }
                hb_list_rem(audios, audio);
                if( audio != NULL)
                    if( audio->out.name )
                    {
                        free( audio->out.name);
                    }
                    free( audio );
            }

            /* Audio Codecs */
            i = 0;
            if( acodecs )
            {
                char * token = strtok(acodecs, ",");
                if( token == NULL )
                    token = acodecs;
                while ( token != NULL )
                {
                    if ((acodec = get_acodec_for_string(token)) == -1)
                    {
                        fprintf(stderr, "Invalid codec %s, using default for container.\n", token);
                        acodec = default_acodec;
                    }
                    if( i < num_audio_tracks )
                    {
                        audio = hb_list_audio_config_item(job->list_audio, i);
                        audio->out.codec = acodec;
                    }
                    else
                    {
                        hb_audio_config_t * last_audio = hb_list_audio_config_item( job->list_audio, i - 1 );
                        hb_audio_config_t audio;

                        if( last_audio )
                        {
                            fprintf(stderr, "More audio codecs than audio tracks, copying track %i and using encoder %s\n",
                                    i, token);
                            hb_audio_config_init(&audio);
                            audio.in.track = last_audio->in.track;
                            audio.out.track = num_audio_tracks++;
                            audio.out.codec = acodec;
                            hb_audio_add(job, &audio);
                        }
                        else
                        {
                            fprintf(stderr, "Audio codecs and no valid audio tracks, skipping codec %s\n", token);
                        }
                    }
                    token = strtok(NULL, ",");
                    i++;
                }
            }
            if( i < num_audio_tracks )
            {
                /* We have fewer inputs than audio tracks, use the default codec for
                 * this container for the remaining tracks. Unless we only have one input
                 * then use that codec instead.
                 */
                if (i != 1)
                    acodec = default_acodec;
                for ( ; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.codec = acodec;
                }
            }
            /* Audio Codecs */

            /* Sample Rate */
            i = 0;
            if( arates )
            {
                char * token = strtok(arates, ",");
                if (token == NULL)
                    token = arates;
                while ( token != NULL )
                {
                    arate = atoi(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    int j;

                    for( j = 0; j < hb_audio_rates_count; j++ )
                    {
                        if( !strcmp( token, hb_audio_rates[j].string ) )
                        {
                            arate = hb_audio_rates[j].rate;
                            break;
                        }
                    }

                    if( audio != NULL )
                    {
                        if (!is_sample_rate_valid(arate))
                        {
                            fprintf(stderr, "Invalid sample rate %d, using input rate %d\n", arate, audio->in.samplerate);
                            arate = audio->in.samplerate;
                        }
                        
                        audio->out.samplerate = arate;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    }
                    else 
                    {
                        fprintf(stderr, "Ignoring sample rate %d, no audio tracks\n", arate);
                    }
                    token = strtok(NULL, ",");
                }
            }
            if (i < num_audio_tracks)
            {
                /* We have fewer inputs than audio tracks, use default sample rate.
                 * Unless we only have one input, then use that for all tracks.
                 */
                if (i != 1)
                    arate = audio->in.samplerate;
                for ( ; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.samplerate = arate;
                }
            }
            /* Sample Rate */

            /* Audio Bitrate */
            i = 0;
            if( abitrates )
            {
                char * token = strtok(abitrates, ",");
                if (token == NULL)
                    token = abitrates;
                while ( token != NULL )
                {
                    abitrate = atoi(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);

                    if( audio != NULL )
                    {
                        audio->out.bitrate = abitrate;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    }
                    else 
                    {
                        fprintf(stderr, "Ignoring bitrate %d, no audio tracks\n", abitrate);
                    }
                    token = strtok(NULL, ",");
                }
            }
            if (i < num_audio_tracks)
            {
                /* We have fewer inputs than audio tracks, use the default bitrate
                 * for the remaining tracks. Unless we only have one input, then use
                 * that for all tracks.
                 */
                if (i != 1)
                    abitrate = default_abitrate;
                for (; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.bitrate = abitrate;
                }
            }
            /* Audio Bitrate */

            /* Audio DRC */
            i = 0;
            if ( dynamic_range_compression )
            {
                char * token = strtok(dynamic_range_compression, ",");
                if (token == NULL)
                    token = dynamic_range_compression;
                while ( token != NULL )
                {
                    d_r_c = atof(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if( audio != NULL )
                    {
                        audio->out.dynamic_range_compression = d_r_c;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    } 
                    else
                    {
                        fprintf(stderr, "Ignoring drc, no audio tracks\n");
                    }
                    token = strtok(NULL, ",");
                }
            }
            if (i < num_audio_tracks)
            {
                /* We have fewer inputs than audio tracks, use no DRC for the remaining
                 * tracks. Unless we only have one input, then use the same DRC for all
                 * tracks.
                 */
                if (i != 1)
                    d_r_c = 0;
                for (; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.dynamic_range_compression = d_r_c;
                }
            }
            /* Audio DRC */

            /* Audio Mixdown */
            i = 0;
            if ( mixdowns )
            {
                char * token = strtok(mixdowns, ",");
                if (token == NULL)
                    token = mixdowns;
                while ( token != NULL )
                {
                    mixdown = hb_mixdown_get_mixdown_from_short_name(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if( audio != NULL )
                    {
                        audio->out.mixdown = mixdown;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    }
                    else
                    {
                        fprintf(stderr, "Ignoring mixdown, no audio tracks\n");
                    }
                    token = strtok(NULL, ",");
                }
            }
            if (i < num_audio_tracks)
            {
                /* We have fewer inputs than audio tracks, use DPLII for the rest. Unless
                 * we only have one input, then use that.
                 */
                if (i != 1)
                    mixdown = HB_AMIXDOWN_DOLBYPLII;
                for (; i < num_audio_tracks; i++)
                {
                   audio = hb_list_audio_config_item(job->list_audio, i);
                   audio->out.mixdown = mixdown;
                }
            }
            /* Audio Mixdown */

            /* Audio Track Names */
            i = 0;
            if ( anames )
            {
                char * token = strtok(anames, ",");
                if (token == NULL)
                    token = anames;
                while ( token != NULL )
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if( audio != NULL )
                    {
                        audio->out.name = strdup(token);
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more names than audio tracks, oops */
                    }
                    else
                    {
                        fprintf(stderr, "Ignoring aname '%s', no audio track\n",
                                token);
                    }
                    token = strtok(NULL, ",");
                }
            }
            if( i < num_audio_tracks && i == 1 )
            {
                /* We have exactly one name and more than one audio track. Use the same
                 * name for all tracks. */
                for ( ; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.name = strdup(anames);
                }
            }
            /* Audio Track Names */

            if( size )
            {
                job->vbitrate = hb_calc_bitrate( job, size );
                fprintf( stderr, "Calculated bitrate: %d kbps\n",
                         job->vbitrate );
            }

            if( sub )
            {
                hb_subtitle_t *subtitle;
                /* 
                 * Find the subtitle with the same track as "sub" and
                 * add that to the job subtitle list
                 */
                subtitle = hb_list_item( title->list_subtitle, sub-1 );
                if( subtitle ) {
                    if( subtitle_force ) {
                        subtitle->config.force = subtitle_force;
                    }
                    hb_list_add( job->list_subtitle, subtitle );
                } else {
                    fprintf( stderr, "Could not find subtitle track %d, skipped\n", sub );
                }
            }

            if( native_language )
            {
                job->native_language = strdup( native_language );
            }

            if( job->mux )
            {
                job->mux = mux;
            }

            if ( largeFileSize )
            {
                job->largeFileSize = 1;
            }
            if ( mp4_optimize )
            {
                job->mp4_optimize = 1;
            }
            if ( ipod_atom )
            {
                job->ipod_atom = 1;
            }

            job->file = strdup( output );

            if( crf )
            {
                job->crf = 1;
            }
            
            if( color_matrix )
            {
                job->color_matrix = color_matrix;
            }

            if( x264opts != NULL && *x264opts != '\0' )
            {
                job->x264opts = x264opts;
            }
            else /*avoids a bus error crash when options aren't specified*/
            {
                job->x264opts =  NULL;
            }
            if (maxWidth)
                job->maxWidth = maxWidth;
            if (maxHeight)
                job->maxHeight = maxHeight;

            if( start_at_preview )
            {
                job->start_at_preview = start_at_preview - 1;
                job->seek_points = preview_count;
            }
            
            if( stop_at_pts )
            {
                job->pts_to_stop = stop_at_pts;
                subtitle_scan = 0;
            }
            
            if( stop_at_frame )
            {
                job->frame_to_stop = stop_at_frame;
                subtitle_scan = 0;
            }
            
            if( subtitle_scan )
            {
                char *x264opts_tmp;

                /*
                 * When subtitle scan is enabled do a fast pre-scan job
                 * which will determine which subtitles to enable, if any.
                 */
                job->pass = -1;

                x264opts_tmp = job->x264opts;

                job->x264opts = NULL;

                job->indepth_scan = subtitle_scan;
                fprintf( stderr, "Subtitle Scan Enabled - enabling "
                         "subtitles if found for foreign language segments\n");
                job->select_subtitle = malloc(sizeof(hb_subtitle_t*));
                *(job->select_subtitle) = NULL;

                job->select_subtitle_config.dest = RENDERSUB;
                job->select_subtitle_config.default_track = 0;
                job->select_subtitle_config.force = subtitle_force;

                /*
                 * Add the pre-scan job
                 */
                hb_add( h, job );

                job->x264opts = x264opts_tmp;
            }

            if( twoPass )
            {
                /*
                 * If subtitle_scan is enabled then only turn it on
                 * for the first pass and then off again for the
                 * second.
                 */
                hb_subtitle_t **subtitle_tmp = job->select_subtitle;

                job->select_subtitle = NULL;

                job->pass = 1;

                job->indepth_scan = 0;

                if (x264opts)
                {
                    x264opts2 = strdup(x264opts);
                }

                /*
                 * If turbo options have been selected then append them
                 * to the x264opts now (size includes one ':' and the '\0')
                 */
                if( turbo_opts_enabled )
                {
                    int size = (x264opts ? strlen(x264opts) : 0) + strlen(turbo_opts) + 2;
                    char *tmp_x264opts;

                    tmp_x264opts = malloc(size * sizeof(char));
                    if( x264opts )
                    {
                        snprintf( tmp_x264opts, size, "%s:%s",
                                  x264opts, turbo_opts );
                        free( x264opts );
                    } else {
                        /*
                         * No x264opts to modify, but apply the turbo options
                         * anyway as they may be modifying defaults
                         */
                        snprintf( tmp_x264opts, size, "%s",
                                  turbo_opts );
                    }
                    x264opts = tmp_x264opts;

                    fprintf( stderr, "Modified x264 options for pass 1 to append turbo options: %s\n",
                             x264opts );

                    job->x264opts = x264opts;
                }
                hb_add( h, job );

                job->select_subtitle = subtitle_tmp;

                job->pass = 2;
                /*
                 * On the second pass we turn off subtitle scan so that we
                 * can actually encode using any subtitles that were auto
                 * selected in the first pass (using the whacky select-subtitle
                 * attribute of the job).
                 */
                job->indepth_scan = 0;

                job->x264opts = x264opts2;

                hb_add( h, job );
            }
            else
            {
                /*
                 * Turn on subtitle scan if requested, note that this option
                 * precludes encoding of any actual subtitles.
                 */

                job->indepth_scan = 0;
                job->pass = 0;
                hb_add( h, job );
            }
            hb_start( h );
            break;
        }

#define p s.param.working
        case HB_STATE_WORKING:
            fprintf( stdout, "\rEncoding: task %d of %d, %.2f %%",
                     p.job_cur, p.job_count, 100.0 * p.progress );
            if( p.seconds > -1 )
            {
                fprintf( stdout, " (%.2f fps, avg %.2f fps, ETA "
                         "%02dh%02dm%02ds)", p.rate_cur, p.rate_avg,
                         p.hours, p.minutes, p.seconds );
            }
            fflush(stdout);
            break;
#undef p

#define p s.param.muxing
        case HB_STATE_MUXING:
        {
            if (show_mux_warning)
            {
                fprintf( stdout, "\rMuxing: this may take awhile..." );
                fflush(stdout);
                show_mux_warning = 0;
            }
            break;
        }
#undef p

#define p s.param.workdone
        case HB_STATE_WORKDONE:
            /* Print error if any, then exit */
            switch( p.error )
            {
                case HB_ERROR_NONE:
                    fprintf( stderr, "\nRip done!\n" );
                    break;
                case HB_ERROR_CANCELED:
                    fprintf( stderr, "\nRip canceled.\n" );
                    break;
                default:
                    fprintf( stderr, "\nRip failed (error %x).\n",
                             p.error );
            }
            die = 1;
            break;
#undef p
    }
    return 0;
}

/****************************************************************************
 * SigHandler:
 ****************************************************************************/
static volatile int64_t i_die_date = 0;
void SigHandler( int i_signal )
{
    if( die == 0 )
    {
        die = 1;
        i_die_date = hb_get_date();
        fprintf( stderr, "Signal %d received, terminating - do it "
                 "again in case it gets stuck\n", i_signal );
    }
    else if( i_die_date + 500 < hb_get_date() )
    {
        fprintf( stderr, "Dying badly, files might remain in your /tmp\n" );
        exit( 1 );
    }
}

/****************************************************************************
 * ShowHelp:
 ****************************************************************************/
static void ShowHelp()
{
    int i;
    FILE* const out = stdout;

    fprintf( out,
    "Syntax: HandBrakeCLI [options] -i <device> -o <file>\n"
    "\n"
    "### General Handbrake Options------------------------------------------------\n\n"
    "    -h, --help              Print help\n"
    "    -u, --update            Check for updates and exit\n"
    "    -v, --verbose <#>       Be verbose (optional argument: logging level)\n"
    "    -C, --cpu               Set CPU count (default: autodetected)\n"
    "    -Z. --preset <string>   Use a built-in preset. Capitalization matters, and\n"
    "                            if the preset name has spaces, surround it with\n"
    "                            double quotation marks\n"
    "    -z, --preset-list       See a list of available built-in presets\n"
    "        --dvdnav            Use dvdnav (Experimental)\n"
    "\n"

    "### Source Options-----------------------------------------------------------\n\n"
    "    -i, --input <string>    Set input device\n"
    "    -t, --title <number>    Select a title to encode (0 to scan only,\n"
    "                            default: 1)\n"
    "    -L, --longest           Select the longest title\n"
    "    -c, --chapters <string> Select chapters (e.g. \"1-3\" for chapters\n"
    "                            1 to 3, or \"3\" for chapter 3 only,\n"
    "                            default: all chapters)\n"
    "        --angle <number>    Select the DVD angle\n"
    "        --previews <#:B>    Select how many preview images are generated (max 30),\n"
    "                            and whether or not they're stored to disk (0 or 1).\n"
    "                            (default: 10:0)\n"
    "    --start-at-preview <#>  Start encoding at a given preview.\n"
    "    --stop-at     <unit:#>  Stop encoding at a given frame, duration (in seconds),\n"
    "                            or pts (on a 90kHz clock)"
    "\n"

    "### Destination Options------------------------------------------------------\n\n"
    "    -o, --output <string>   Set output file name\n"
    "    -f, --format <string>   Set output format (avi/mp4/ogm/mkv, default:\n"
    "                            autodetected from file name)\n"
    "    -m, --markers           Add chapter markers (mp4 and mkv output formats only)\n"
    "    -4, --large-file        Use 64-bit mp4 files that can hold more than\n"
    "                            4 GB. Note: Breaks iPod, PS3 compatibility.\n"""
    "    -O, --optimize          Optimize mp4 files for HTTP streaming\n"
    "    -I, --ipod-atom         Mark mp4 files so 5.5G iPods will accept them\n"
    "\n"


    "### Video Options------------------------------------------------------------\n\n"
    "    -e, --encoder <string>  Set video library encoder (ffmpeg,x264,theora)\n"
    "                            (default: ffmpeg)\n"
    "    -x, --x264opts <string> Specify advanced x264 options in the\n"
    "                            same style as mencoder:\n"
    "                            option1=value1:option2=value2\n"
    "    -q, --quality <float>   Set video quality (0.0..1.0)\n"
    "    -Q, --cqp               Use with -q for CQP instead of CRF\n"
    "    -S, --size <MB>         Set target size\n"
    "    -b, --vb <kb/s>         Set video bitrate (default: 1000)\n"
    "    -2, --two-pass          Use two-pass mode\n"
    "    -T, --turbo             When using 2-pass use the turbo options\n"
    "                            on the first pass to improve speed\n"
    "                            (only works with x264, affects PSNR by about 0.05dB,\n"
    "                            and increases first pass speed two to four times)\n"
    "    -r, --rate              Set video framerate (" );
    for( i = 0; i < hb_video_rates_count; i++ )
    {
        fprintf( out, hb_video_rates[i].string );
        if( i != hb_video_rates_count - 1 )
            fprintf( out, "/" );
    }
    fprintf( out, ")\n"
    "                            Be aware that not specifying a framerate lets\n"
    "                            HandBrake preserve a source's time stamps,\n"
    "                            potentially creating variable framerate video\n"
    "    --vfr, --cfr, --pfr     Select variable, constant or peak-limited\n"
    "                            frame rate control. VFR preserves the source\n"
    "                            timing. CFR makes the output constant rate at\n"
    "                            the rate given by the -r flag (or the source's\n"
    "                            average rate if no -r is given). PFR doesn't\n"
    "                            allow the rate to go over the rate specified\n"
    "                            with the -r flag but won't change the source\n"
    "                            timing if it's below that rate.\n"
    "                            If none of these flags are given, the default\n"
    "                            is --cfr when -r is given and --vfr otherwise\n"

    "\n"
    "### Audio Options-----------------------------------------------------------\n\n"
    "    -a, --audio <string>    Select audio track(s), separated by commas\n"
    "                            More than one output track can be used for one\n"
    "                            input.\n"
    "                            (\"none\" for no audio, \"1,2,3\" for multiple\n"
    "                             tracks, default: first one)\n"
    "    -E, --aencoder <string> Audio encoder(s) (faac/lame/vorbis/ac3/dts) \n"
    "                            ac3 and dts meaning passthrough\n"
    "                            Separated by commas for more than one audio track.\n"
    "                            (default: guessed)\n"
    "    -B, --ab <kb/s>         Set audio bitrate(s)  (default: 160)\n"
    "                            Separated by commas for more than one audio track.\n"
    "    -6, --mixdown <string>  Format(s) for surround sound downmixing\n"
    "                            Separated by commas for more than one audio track.\n"
    "                            (mono/stereo/dpl1/dpl2/6ch, default: dpl2)\n"
    "    -R, --arate             Set audio samplerate(s) (" );
    for( i = 0; i < hb_audio_rates_count; i++ )
    {
        fprintf( out, hb_audio_rates[i].string );
        if( i != hb_audio_rates_count - 1 )
            fprintf( out, "/" );
    }
    fprintf( out, " kHz)\n"
    "                            Separated by commas for more than one audio track.\n"
    "    -D, --drc <float>       Apply extra dynamic range compression to the audio,\n"
    "                            making soft sounds louder. Range is 1.0 to 4.0\n"
    "                            (too loud), with 1.5 - 2.5 being a useful range.\n"
    "                            Separated by commas for more than one audio track.\n"
    "    -A, --aname <string>    Audio track name(s),\n"
    "                            Separated by commas for more than one audio track.\n"
    "\n"

    "### Picture Settings---------------------------------------------------------\n\n"
    "    -w, --width <number>    Set picture width\n"
    "    -l, --height <number>   Set picture height\n"
    "        --crop <T:B:L:R>    Set cropping values (default: autocrop)\n"
    "    -Y, --maxHeight <#>     Set maximum height\n"
    "    -X, --maxWidth <#>      Set maximum width\n"
    "    -p, --pixelratio        Store pixel aspect ratio in video stream\n"
    "    -P, --loosePixelratio   Store pixel aspect ratio with specified width\n"
    "          <MOD:PARX:PARY>   Takes as optional arguments what number you want\n"
    "                            the dimensions to divide cleanly by (default 16)\n"
    "                            and the pixel ratio to use (default autodetected)\n"
    "    -M  --color-matrix      Set the color space signaled by the output\n"
    "          <601 or 709>      (Bt.601 is mostly for SD content, Bt.709 for HD,\n"
    "                             default: set by resolution)\n"
    "\n"

    "### Filters---------------------------------------------------------\n\n"

     "    -d, --deinterlace       Deinterlace video with yadif/mcdeint filter\n"
     "          <YM:FD:MM:QP>     (default 0:-1:-1:1)\n"
     "           or\n"
     "          <fast/slow/slower>\n"
     "    -5, --decomb            Selectively deinterlaces when it detects combing\n"
     "          <MO:ME:MT:ST:BT:BX:BY>     (default: 1:2:6:9:80:16:16)\n"
     "    -9, --detelecine        Detelecine (ivtc) video with pullup filter\n"
     "                            Note: this filter drops duplicate frames to\n"
     "                            restore the pre-telecine framerate, unless you\n"
     "                            specify a constant framerate (--rate 29.97)\n"
     "          <L:R:T:B:SB:MP>   (default 1:1:4:4:0:0)\n"
     "    -8, --denoise           Denoise video with hqdn3d filter\n"
     "          <SL:SC:TL:TC>     (default 4:3:6:4.5)\n"
     "           or\n"
     "          <weak/medium/strong>\n"
     "    -7, --deblock           Deblock video with pp7 filter\n"
     "          <QP:M>            (default 5:2)\n"
    "    -g, --grayscale         Grayscale encoding\n"
    "\n"

    "### Subtitle Options------------------------------------------------------------\n\n"
    "    -s, --subtitle <number> Select subtitle (default: none)\n"
    "    -U, --subtitle-scan     Scan for subtitles in an extra 1st pass, and choose\n"
    "                            the one that's only used 10 percent of the time\n"
    "                            or less. This should locate subtitles for short\n"
    "                            foreign language segments. Best used in conjunction\n"
    "                            with --subtitle-forced.\n"
    "    -F, --subtitle-forced   Only display subtitles from the selected stream if\n"
    "                            the subtitle has the forced flag set. May be used in\n"
    "                            conjunction with --subtitle-scan to auto-select\n"
    "                            a stream if it contains forced subtitles.\n"
    "    -N, --native-language   Select subtitles with this language if it does not\n"
    "          <string>          match the Audio language. Provide the language's\n"
    "                            iso639-2 code (fre, eng, spa, dut, et cetera)\n"


    "\n"


    );
}

/****************************************************************************
 * ShowPresets:
 ****************************************************************************/
static void ShowPresets()
{
    printf("\n< Apple\n");

    printf("\n   + Universal:  -e x264  -q 0.589999973773956 -a 1,1 -E faac,ac3 -B 160,auto -R 48,Auto -6 dpl2,auto -f mp4 -X 720 -P -m -x level=30:cabac=0:ref=3:mixed-refs=1:analyse=all:me=umh:no-fast-pskip=1\n");

    printf("\n   + iPod:  -e x264  -b 700 -a 1 -E faac -B 160 -R 48 -6 dpl2 -f mp4 -I -X 320 -m -x level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1\n");

    printf("\n   + iPhone & iPod Touch:  -e x264  -q 0.589999973773956 -a 1 -E faac -B 128 -R 48 -6 dpl2 -f mp4 -X 480 -m -x level=30:cabac=0:ref=2:mixed-refs:analyse=all:me=umh:no-fast-pskip=1\n");

    printf("\n   + AppleTV:  -e x264  -q 0.589999973773956 -a 1,1 -E faac,ac3 -B 160,auto -R 48,Auto -6 dpl2,auto -f mp4 -4 -X 960 -P -m -x level=30:cabac=0:ref=3:mixed-refs=1:bframes=6:weightb=1:direct=auto:no-fast-pskip=1:me=umh:subq=7:analyse=all\n");

    printf("\n   + QuickTime:  -e x264  -b 1800 -a 1 -E faac -B 160 -R Auto -6 dpl2 -f mp4 -p -m -2 -T -x ref=3:mixed-refs:bframes=3:weightb:direct=auto:me=umh:subme=7:analyse=all:8x8dct:trellis=1:no-fast-pskip=1:psy-rd=1,1\n");

    printf("\n   << Legacy\n");

    printf("\n      + AppleTV Legacy:  -e x264  -b 2500 -a 1,1 -E faac,ac3 -B 160,auto -R 48,Auto -6 dpl2,auto -f mp4 -4 -p -m -x bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:trellis=1:cabac=0\n");

    printf("\n      + iPhone Legacy:  -e x264  -b 960 -a 1 -E faac -B 128 -R 48 -6 dpl2 -f mp4 -I -X 480 -m -x level=30:cabac=0:ref=1:analyse=all:me=umh:no-fast-pskip=1:trellis=1\n");

    printf("\n      + iPod Legacy:  -e x264  -b 1500 -a 1 -E faac -B 160 -R 48 -6 dpl2 -f mp4 -I -X 640 -m -x level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1\n");

    printf("\n   >>\n");

    printf("\n>\n");

    printf("\n< Basic\n");

    printf("\n   + Normal:  -e x264  -b 1500 -a 1 -E faac -B 160 -R Auto -6 dpl2 -f mp4 -p -m -2 -T -x ref=2:bframes=2:me=umh\n");

    printf("\n   + Classic:  -b 1000 -a 1 -E faac -B 160 -R Auto -6 dpl2 -f mp4\n");

    printf("\n>\n");

    printf("\n< High Profile\n");

    printf("\n   + Animation:  -e x264  -b 1000 -a 1 -E faac -B 160 -R Auto -6 dpl2 -f mkv --detelecine --decomb -p -m -2 -T -x ref=5:mixed-refs:bframes=6:weightb:direct=auto:b-pyramid:me=umh:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip:filter=2,2:psy-rd=1,1:subme=9\n");

    printf("\n   + Constant Quality Rate:  -e x264  -q 0.600000023841858 -a 1 -E ac3 -B 160 -R Auto -6 auto -f mkv -p -m -x ref=3:mixed-refs:bframes=3:b-pyramid:weightb:filter=-2,-1:trellis=1:analyse=all:8x8dct:me=umh:subme=9:psy-rd=1,1\n");

    printf("\n   + Film:  -e x264  -b 1800 -a 1 -E ac3 -B 160 -R Auto -6 auto -f mkv -p -m -2 -T -x ref=3:mixed-refs:bframes=6:weightb:direct=auto:b-pyramid:me=umh:subme=9:analyse=all:8x8dct:trellis=1:no-fast-pskip:psy-rd=1,1\n");

    printf("\n   + Television:  -e x264  -b 1300 -a 1 -E faac -B 160 -R Auto -6 dpl2 -f mkv --detelecine --decomb -p -m -2 -T -x ref=3:mixed-refs:bframes=6:weightb:direct=auto:b-pyramid:me=umh:subme=9:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip=1:psy-rd=1,1\n");

    printf("\n>\n");

    printf("\n< Gaming Consoles\n");

    printf("\n   + PSP:  -b 1024 -a 1 -E faac -B 128 -R 48 -6 dpl2 -f mp4 -X 368 -Y 208 -m\n");

    printf("\n   + PS3:  -e x264  -b 2500 -a 1 -E faac -B 160 -R 48 -6 dpl2 -f mp4 --crop 0:0:0:0 -p -x level=41:me=umh\n");

    printf("\n   + Xbox 360:  -e x264  -b 2000 -a 1 -E faac -B 160 -R 48 -6 dpl2 -f mp4 -p -x level=40:ref=2:mixed-refs:bframes=3:weightb:subme=9:direct=auto:b-pyramid:me=umh:analyse=all:no-fast-pskip:filter=-2,-1\n");

    printf("\n>\n");

}

/****************************************************************************
 * ParseOptions:
 ****************************************************************************/
static int ParseOptions( int argc, char ** argv )
{
    
    #define PREVIEWS 257
    #define START_AT_PREVIEW 258
    #define STOP_AT 259
    #define ANGLE 260
    #define DVDNAV 261
    
    for( ;; )
    {
        static struct option long_options[] =
          {
            { "help",        no_argument,       NULL,    'h' },
            { "update",      no_argument,       NULL,    'u' },
            { "verbose",     optional_argument, NULL,    'v' },
            { "cpu",         required_argument, NULL,    'C' },
            { "dvdnav",      no_argument,       NULL,    DVDNAV },

            { "format",      required_argument, NULL,    'f' },
            { "input",       required_argument, NULL,    'i' },
            { "output",      required_argument, NULL,    'o' },
            { "large-file",  no_argument,       NULL,    '4' },
            { "optimize",    no_argument,       NULL,    'O' },
            { "ipod-atom",   no_argument,       NULL,    'I' },

            { "title",       required_argument, NULL,    't' },
            { "longest",     no_argument,       NULL,    'L' },
            { "chapters",    required_argument, NULL,    'c' },
            { "angle",       required_argument, NULL,    ANGLE },
            { "markers",     optional_argument, NULL,    'm' },
            { "audio",       required_argument, NULL,    'a' },
            { "mixdown",     required_argument, NULL,    '6' },
            { "drc",         required_argument, NULL,    'D' },
            { "subtitle",    required_argument, NULL,    's' },
            { "subtitle-scan", no_argument,     NULL,    'U' },
            { "subtitle-forced", no_argument,   NULL,    'F' },
            { "native-language", required_argument, NULL,'N' },

            { "encoder",     required_argument, NULL,    'e' },
            { "aencoder",    required_argument, NULL,    'E' },
            { "two-pass",    no_argument,       NULL,    '2' },
            { "deinterlace", optional_argument, NULL,    'd' },
            { "deblock",     optional_argument, NULL,    '7' },
            { "denoise",     optional_argument, NULL,    '8' },
            { "detelecine",  optional_argument, NULL,    '9' },
            { "decomb",      optional_argument, NULL,    '5' },
            { "grayscale",   no_argument,       NULL,    'g' },
            { "pixelratio",  no_argument,       NULL,    'p' },
            { "loosePixelratio", optional_argument,   NULL,    'P' },
            { "width",       required_argument, NULL,    'w' },
            { "height",      required_argument, NULL,    'l' },
            { "crop",        required_argument, NULL,    'n' },

            { "vb",          required_argument, NULL,    'b' },
            { "quality",     required_argument, NULL,    'q' },
            { "size",        required_argument, NULL,    'S' },
            { "ab",          required_argument, NULL,    'B' },
            { "rate",        required_argument, NULL,    'r' },
            { "arate",       required_argument, NULL,    'R' },
            { "cqp",         no_argument,       NULL,    'Q' },
            { "x264opts",    required_argument, NULL,    'x' },
            { "turbo",       no_argument,       NULL,    'T' },
            { "maxHeight",   required_argument, NULL,    'Y' },
            { "maxWidth",    required_argument, NULL,    'X' },
            { "preset",      required_argument, NULL,    'Z' },
            { "preset-list", no_argument,       NULL,    'z' },

            { "aname",       required_argument, NULL,    'A' },
            { "color-matrix",required_argument, NULL,    'M' },
            { "previews",    required_argument, NULL,    PREVIEWS },
            { "start-at-preview", required_argument, NULL, START_AT_PREVIEW },
            { "stop-at",    required_argument, NULL,     STOP_AT },
            { "vfr",         no_argument,       &cfr,    0 },
            { "cfr",         no_argument,       &cfr,    1 },
            { "pfr",         no_argument,       &cfr,    2 },
            { 0, 0, 0, 0 }
          };

        int option_index = 0;
        int c;

		c = getopt_long( argc, argv,
						 "hv::uC:f:4i:Io:t:Lc:m::M:a:A:6:s:UFN:e:E:2dD:7895gpOP::w:l:n:b:q:S:B:r:R:Qx:TY:X:Z:z",
                         long_options, &option_index );
        if( c < 0 )
        {
            break;
        }

        switch( c )
        {
            case 0:
                /* option was handled entirely in getopt_long */
                break;
            case 'h':
                ShowHelp();
                exit( 0 );
            case 'u':
                update = 1;
                break;
            case 'v':
                if( optarg != NULL )
                {
                    debug = atoi( optarg );
                }
                else
                {
                    debug = 1;
                }
                break;
            case 'C':
                cpu = atoi( optarg );
                break;

            case 'Z':
                preset = 1;
                preset_name = strdup(optarg);
                break;
            case 'z':
                ShowPresets();
                exit ( 0 );
            case DVDNAV:
                dvdnav = 1;
                break;

            case 'f':
                format = strdup( optarg );
                break;
            case 'i':
                input = strdup( optarg );
#ifdef __APPLE_CC__
                char *devName = bsd_name_for_path( input ); // alloc
                if( devName )
                {
                    if( device_is_dvd( devName ))
                    {
                        free( input );
                        input = malloc( strlen( "/dev/" ) + strlen( devName ) + 1 );
                        sprintf( input, "/dev/%s", devName );
                    }
                    free( devName );
                }
#endif
                break;
            case 'o':
                output = strdup( optarg );
                break;
            case '4':
                largeFileSize = 1;
                break;
            case 'O':
                mp4_optimize = 1;
                break;
            case 'I':
                ipod_atom = 1;
                break;

            case 't':
                titleindex = atoi( optarg );
                break;
            case 'L':
                longest_title = 1;
                break;
            case 'c':
            {
                int start, end;
                if( sscanf( optarg, "%d-%d", &start, &end ) == 2 )
                {
                    chapter_start = start;
                    chapter_end   = end;
                }
                else if( sscanf( optarg, "%d", &start ) == 1 )
                {
                    chapter_start = start;
                    chapter_end   = chapter_start;
                }
                else
                {
                    fprintf( stderr, "chapters: invalid syntax (%s)\n",
                             optarg );
                    return -1;
                }
                break;
            }
            case ANGLE:
                angle = atoi( optarg );
                break;
            case 'm':
                if( optarg != NULL )
                {
                    marker_file = strdup( optarg );
                }
                chapter_markers = 1;
                break;
            case 'a':
                if( optarg != NULL )
                {
                    atracks = strdup( optarg );
                }
                else
                {
                    atracks = "1" ;
                }
                break;
            case '6':
                if( optarg != NULL )
                {
                    mixdowns = strdup( optarg );
                }
                break;
            case 'D':
                if( optarg != NULL )
                {
                    dynamic_range_compression = strdup( optarg );
                }
                break;
            case 's':
                sub = atoi( optarg );
                break;
            case 'U':
                subtitle_scan = 1;
                break;
            case 'F':
                subtitle_force = 1;
                break;
            case 'N':
                native_language = strdup( optarg );
                break;
            case '2':
                twoPass = 1;
                break;
            case 'd':
                if( optarg != NULL )
                {
                    if (!( strcmp( optarg, "fast" ) ))
                    {
                        deinterlace_opt = "-1";
                    }
                    else if (!( strcmp( optarg, "slow" ) ))
                    {
                        deinterlace_opt = "2";
                    }
                    else if (!( strcmp( optarg, "slower" ) ))
                    {
                        deinterlace_opt = "0";
                    }
                    else
                    {
                        deinterlace_opt = strdup( optarg );
                    }
                }
                deinterlace = 1;
                break;
            case '7':
                if( optarg != NULL )
                {
                    deblock_opt = strdup( optarg );
                }
                deblock = 1;
                break;
            case '8':
                if( optarg != NULL )
                {
                    if (!( strcmp( optarg, "weak" ) ))
                    {
                        denoise_opt = "2:1:2:3";
                    }
                    else if (!( strcmp( optarg, "medium" ) ))
                    {
                        denoise_opt = "3:2:2:3";
                    }
                    else if (!( strcmp( optarg, "strong" ) ))
                    {
                        denoise_opt = "7:7:5:5";
                    }
                    else
                    {
                        denoise_opt = strdup( optarg );
                    }
                }
                denoise = 1;
                break;
            case '9':
                if( optarg != NULL )
                {
                    detelecine_opt = strdup( optarg );
                }
                detelecine = 1;
                break;
            case '5':
                if( optarg != NULL )
                {
                    decomb_opt = strdup( optarg );
                }
                decomb = 1;
                break;
            case 'g':
                grayscale = 1;
                break;
            case 'p':
                pixelratio = 1;
                break;
            case 'P':
                loosePixelratio = 1;
                if( optarg != NULL )
                {
                    sscanf( optarg, "%i:%i:%i", &modulus, &par_width, &par_height );
                }
                break;
            case 'e':
                if( !strcasecmp( optarg, "ffmpeg" ) )
                {
                    vcodec = HB_VCODEC_FFMPEG;
                }
                else if( !strcasecmp( optarg, "x264" ) )
                {
                    vcodec = HB_VCODEC_X264;
                }
                else if( !strcasecmp( optarg, "x264b13" ) )
                {
                    vcodec = HB_VCODEC_X264;
                    h264_13 = 1;
                }
                else if( !strcasecmp( optarg, "x264b30" ) )
                {
                    vcodec = HB_VCODEC_X264;
                    h264_30 = 1;
                }
                else if( !strcasecmp( optarg, "theora" ) )
                {
                    vcodec = HB_VCODEC_THEORA;
                }
                else
                {
                    fprintf( stderr, "invalid codec (%s)\n", optarg );
                    return -1;
                }
                break;
            case 'E':
                if( optarg != NULL )
                {
                    acodecs = strdup( optarg );
                }
                break;
            case 'w':
                width = atoi( optarg );
                break;
            case 'l':
                height = atoi( optarg );
                break;
            case 'n':
            {
                int    i;
                char * tmp = optarg;
                for( i = 0; i < 4; i++ )
                {
                    if( !*tmp )
                        break;
                    crop[i] = strtol( tmp, &tmp, 0 );
                    tmp++;
                }
                break;
            }
            case 'r':
            {
                int i;
                vrate = 0;
                for( i = 0; i < hb_video_rates_count; i++ )
                {
                    if( !strcmp( optarg, hb_video_rates[i].string ) )
                    {
                        vrate = hb_video_rates[i].rate;
                        break;
                    }
                }
                if( !vrate )
                {
                    fprintf( stderr, "invalid framerate %s\n", optarg );
                }
                else if ( cfr == 0 )
                {
                    cfr = 1;
                }
                break;
            }
            case 'R':
                if( optarg != NULL )
                {
                    arates = strdup( optarg );
                }
                break;
            case 'b':
                vbitrate = atoi( optarg );
                break;
            case 'q':
                vquality = atof( optarg );
                break;
            case 'S':
                size = atoi( optarg );
                break;
            case 'B':
                if( optarg != NULL )
                {
                    abitrates = strdup( optarg );
                }
                break;
            case 'Q':
                crf = 0;
                break;
            case 'x':
                x264opts = strdup( optarg );
                break;
            case 'T':
                turbo_opts_enabled = 1;
                break;
            case 'Y':
                maxHeight = atoi( optarg );
                break;
            case 'X':
                maxWidth = atoi (optarg );
                break;
            case 'A':
                if( optarg != NULL )
                {
                    anames = strdup( optarg );
                }
                break;
            case PREVIEWS:
                sscanf( optarg, "%i:%i", &preview_count, &store_previews );
                break;
            case START_AT_PREVIEW:
                start_at_preview = atoi( optarg );
                break;
            case STOP_AT:
                stop_at_string = strdup( optarg );
                stop_at_token = strtok( stop_at_string, ":");
                if( !strcmp( stop_at_token, "frame" ) )
                {
                    stop_at_token = strtok( NULL, ":");
                    stop_at_frame = atoi(stop_at_token);
                }
                else if( !strcmp( stop_at_token, "pts" ) )
                {
                    stop_at_token = strtok( NULL, ":");
                    sscanf( stop_at_token, "%"SCNd64, &stop_at_pts );
                }
                else if( !strcmp( stop_at_token, "duration" ) )
                {
                    stop_at_token = strtok( NULL, ":");
                    sscanf( stop_at_token, "%"SCNd64, &stop_at_pts );
                    stop_at_pts *= 90000LL;
                }
                break;
            case 'M':
                if( atoi( optarg ) == 601 )
                    color_matrix = 1;
                else if( atoi( optarg ) == 709 )
                    color_matrix = 2;
                break;
            default:
                fprintf( stderr, "unknown option (%s)\n", argv[optind] );
                return -1;
        }
    }

    return 0;
}

static int CheckOptions( int argc, char ** argv )
{
    if( update )
    {
        return 0;
    }

    if( input == NULL || *input == '\0' )
    {
        fprintf( stderr, "Missing input device. Run %s --help for "
                 "syntax.\n", argv[0] );
        return 1;
    }

    /* Parse format */
    if( titleindex > 0 )
    {
        if( output == NULL || *output == '\0' )
        {
            fprintf( stderr, "Missing output file name. Run %s --help "
                     "for syntax.\n", argv[0] );
            return 1;
        }

        if( !format )
        {
            char * p = strrchr( output, '.' );

            /* autodetect */
            if( p && !strcasecmp( p, ".avi" ) )
            {
                mux = HB_MUX_AVI;
                default_acodec = HB_ACODEC_LAME;
            }
            else if( p && ( !strcasecmp( p, ".mp4" )  ||
                            !strcasecmp( p, ".m4v" ) ) )
            {
                if ( h264_30 == 1 )
                    mux = HB_MUX_IPOD;
                else
                    mux = HB_MUX_MP4;
                default_acodec = HB_ACODEC_FAAC;
            }
            else if( p && ( !strcasecmp( p, ".ogm" ) ||
                            !strcasecmp( p, ".ogg" ) ) )
            {
                mux = HB_MUX_OGM;
                default_acodec = HB_ACODEC_VORBIS;
            }
            else if( p && !strcasecmp(p, ".mkv" ) )
            {
                mux = HB_MUX_MKV;
                default_acodec = HB_ACODEC_AC3;
            }
            else
            {
                fprintf( stderr, "Output format couldn't be guessed "
                         "from file name, using default.\n" );
                return 0;
            }
        }
        else if( !strcasecmp( format, "avi" ) )
        {
            mux = HB_MUX_AVI;
            default_acodec = HB_ACODEC_LAME;
        }
        else if( !strcasecmp( format, "mp4" ) ||
                 !strcasecmp( format, "m4v" ) )
        {
            if ( h264_30 == 1)
                mux = HB_MUX_IPOD;
            else
                mux = HB_MUX_MP4;
            default_acodec = HB_ACODEC_FAAC;
        }
        else if( !strcasecmp( format, "ogm" ) ||
                 !strcasecmp( format, "ogg" ) )
        {
            mux = HB_MUX_OGM;
            default_acodec = HB_ACODEC_VORBIS;
        }
        else if( !strcasecmp( format, "mkv" ) )
        {
            mux = HB_MUX_MKV;
            default_acodec = HB_ACODEC_AC3;
        }
        else
        {
            fprintf( stderr, "Invalid output format (%s). Possible "
                     "choices are avi, mp4, m4v, ogm, ogg and mkv\n.", format );
            return 1;
        }
    }

    return 0;
}

static int get_acodec_for_string( char *codec )
{
    if( !strcasecmp( codec, "ac3" ) )
    {
        return HB_ACODEC_AC3;
    }
    else if( !strcasecmp( codec, "dts" ) || !strcasecmp( codec, "dca" ) )
    {
        return HB_ACODEC_DCA;
    }
    else if( !strcasecmp( codec, "lame" ) )
    {
        return HB_ACODEC_LAME;
    }
    else if( !strcasecmp( codec, "faac" ) )
    {
        return HB_ACODEC_FAAC;
    }
    else if( !strcasecmp( codec, "vorbis") )
    {
        return HB_ACODEC_VORBIS;
    }
#ifdef __APPLE__
    else if( !strcasecmp( codec, "ca_aac") )
    {
        return HB_ACODEC_CA_AAC;
    }
#endif
    else
    {
        return -1;
    }
}

static int is_sample_rate_valid(int rate)
{
    int i;
    for( i = 0; i < hb_audio_rates_count; i++ )
    {
            if (rate == hb_audio_rates[i].rate)
                return 1;
    }
    return 0;
}

#ifdef __APPLE_CC__
/****************************************************************************
 * bsd_name_for_path
 *
 * Returns the BSD device name for the block device that contains the
 * passed-in path. Returns NULL on failure.
 ****************************************************************************/
static char* bsd_name_for_path(char *path)
{
    OSStatus err;
    FSRef ref;
    err = FSPathMakeRef( (const UInt8 *) input, &ref, NULL );
    if( err != noErr )
    {
        return NULL;
    }

    // Get the volume reference number.
    FSCatalogInfo catalogInfo;
    err = FSGetCatalogInfo( &ref, kFSCatInfoVolume, &catalogInfo, NULL, NULL,
                            NULL);
    if( err != noErr )
    {
        return NULL;
    }
    FSVolumeRefNum volRefNum = catalogInfo.volume;

    // Now let's get the device name
    GetVolParmsInfoBuffer volumeParms;
    err = FSGetVolumeParms( volRefNum, &volumeParms, sizeof( volumeParms ) );
    if( err != noErr )
    {
        return NULL;
    }

    // A version 4 GetVolParmsInfoBuffer contains the BSD node name in the vMDeviceID field.
    // It is actually a char * value. This is mentioned in the header CoreServices/CarbonCore/Files.h.
    if( volumeParms.vMVersion < 4 )
    {
        return NULL;
    }

    // vMDeviceID might be zero as is reported with experimental ZFS (zfs-119) support in Leopard.
    if( !volumeParms.vMDeviceID )
    {
        return NULL;
    }

    return strdup( volumeParms.vMDeviceID );
}

/****************************************************************************
 * device_is_dvd
 *
 * Returns whether or not the passed in BSD device represents a DVD, or other
 * optical media.
 ****************************************************************************/
static int device_is_dvd(char *device)
{
    io_service_t service = get_iokit_service(device);
    if( service == IO_OBJECT_NULL )
    {
        return 0;
    }
    int result = is_dvd_service(service);
    IOObjectRelease(service);
    return result;
}

/****************************************************************************
 * get_iokit_service
 *
 * Returns the IOKit service object for the passed in BSD device name.
 ****************************************************************************/
static io_service_t get_iokit_service( char *device )
{
    CFMutableDictionaryRef matchingDict;
    matchingDict = IOBSDNameMatching( kIOMasterPortDefault, 0, device );
    if( matchingDict == NULL )
    {
        return IO_OBJECT_NULL;
    }
    // Fetch the object with the matching BSD node name. There should only be
    // one match, so IOServiceGetMatchingService is used instead of
    // IOServiceGetMatchingServices to simplify the code.
    return IOServiceGetMatchingService( kIOMasterPortDefault, matchingDict );
}

/****************************************************************************
 * is_dvd_service
 *
 * Returns whether or not the service passed in is a DVD.
 *
 * Searches for an IOMedia object that represents the entire (whole) media that
 * the volume is on. If the volume is on partitioned media, the whole media
 * object will be a parent of the volume's media object. If the media is not
 * partitioned, the volume's media object will be the whole media object.
 ****************************************************************************/
static int is_dvd_service( io_service_t service )
{
    kern_return_t  kernResult;
    io_iterator_t  iter;

    // Create an iterator across all parents of the service object passed in.
    kernResult = IORegistryEntryCreateIterator( service,
                                                kIOServicePlane,
                                                kIORegistryIterateRecursively | kIORegistryIterateParents,
                                                &iter );
    if( kernResult != KERN_SUCCESS )
    {
        return 0;
    }
    if( iter == IO_OBJECT_NULL )
    {
        return 0;
    }

    // A reference on the initial service object is released in the do-while
    // loop below, so add a reference to balance.
    IOObjectRetain( service );

    int result = 0;
    do
    {
        if( is_whole_media_service( service ) &&
            IOObjectConformsTo( service, kIODVDMediaClass) )
        {
            result = 1;
        }
        IOObjectRelease( service );
    } while( !result && (service = IOIteratorNext( iter )) );
    IOObjectRelease( iter );

    return result;
}

/****************************************************************************
 * is_whole_media_service
 *
 * Returns whether or not the service passed in is an IOMedia service and
 * represents the "whole" media instead of just a partition.
 *
 * The whole media object is indicated in the IORegistry by the presence of a
 * property with the key "Whole" and value "Yes".
 ****************************************************************************/
static is_whole_media_service( io_service_t service )
{
    int result = 0;

    if( IOObjectConformsTo( service, kIOMediaClass ) )
    {
        CFTypeRef wholeMedia = IORegistryEntryCreateCFProperty( service,
                                                                CFSTR( kIOMediaWholeKey ),
                                                                kCFAllocatorDefault,
                                                                0 );
        if ( !wholeMedia )
        {
            return 0;
        }
        result = CFBooleanGetValue( (CFBooleanRef)wholeMedia );
        CFRelease( wholeMedia );
    }

    return result;
}
#endif // __APPLE_CC__
