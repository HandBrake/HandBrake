/* $Id: test.c,v 1.82 2005/11/19 08:25:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "hb.h"
#include "parsecsv.h"

/* Options */
static int    debug       = HB_DEBUG_NONE;
static int    update      = 0;
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
static int    grayscale   = 0;
static int    vcodec      = HB_VCODEC_FFMPEG;
static int    h264_13     = 0;
static int    h264_30     = 0;
static char * audios      = NULL;
static int    audio_mixdown = HB_AMIXDOWN_DOLBYPLII;
static float  dynamic_range_compression = 0;
static int    sub         = 0;
static int    width       = 0;
static int    height      = 0;
static int    crop[4]     = { -1,-1,-1,-1 };
static int    cpu         = 0;
static int    vrate       = 0;
static int    arate       = 0;
static float  vquality    = -1.0;
static int    vbitrate    = 0;
static int    size        = 0;
static int    abitrate    = 0;
static int    mux         = 0;
static int    acodec      = 0;
static int    pixelratio  = 0;
static int    loosePixelratio = 0;
static int    modulus       = 0;
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
static int    vfr           = 0;
static int    mp4_optimize  = 0;
static int    ipod_atom     = 0;

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

/****************************************************************************
 * hb_error_handler
 * 
 * When using the CLI just display using hb_log as we always did in the past
 * make sure that we prefix with a nice ERROR message to catch peoples eyes.
 ****************************************************************************/
static void hb_cli_error_handler ( const char *errmsg )
{
    fprintf( stderr, "ERROR: %s", errmsg );
}

int main( int argc, char ** argv )
{
    hb_handle_t * h;
    int           build;
    char        * version;

    /* Parse command line */
    if( ParseOptions( argc, argv ) ||
        CheckOptions( argc, argv ) )
    {
        return 1;
    }

    /* Register our error handler */
    hb_register_error_handler(&hb_cli_error_handler);

    /* Init libhb */
    h = hb_init( debug, update );

    /* Show version */
    fprintf( stderr, "HandBrake %s (%d) - http://handbrake.m0k.org/\n",
             hb_get_version( h ), hb_get_build( h ) );

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
    hb_scan( h, input, titleindex );

    /* Wait... */
    while( !die )
    {
#if !defined(SYS_BEOS)
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
    if( audios ) free( audios );
    if (native_language ) free (native_language );
	if( x264opts ) free (x264opts );
	if( x264opts2 ) free (x264opts2 );
    if (preset_name) free (preset_name);
	
    fprintf( stderr, "HandBrake has exited.\n" );

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
    hb_audio_t    * audio;
    hb_subtitle_t * subtitle;
    int i;

    fprintf( stderr, "+ title %d:\n", title->index );
    fprintf( stderr, "  + vts %d, ttn %d, cells %d->%d (%d blocks)\n",
             title->vts, title->ttn, title->cell_start, title->cell_end,
             title->block_count );
    fprintf( stderr, "  + duration: %02d:%02d:%02d\n",
             title->hours, title->minutes, title->seconds );
    fprintf( stderr, "  + size: %dx%d, aspect: %.2f, %.3f fps\n",
             title->width, title->height,
             (float) title->aspect / HB_ASPECT_BASE,
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
        audio = hb_list_item( title->list_audio, i );
        if( ( audio->codec & HB_ACODEC_AC3 ) || ( audio->codec & HB_ACODEC_DCA) )
        {
            fprintf( stderr, "    + %d, %s, %dHz, %dbps\n", i + 1,
                     audio->lang, audio->rate, audio->bitrate );
        }
        else
        {
            fprintf( stderr, "    + %d, %s\n", i + 1, audio->lang );
        }
    }
    fprintf( stderr, "  + subtitle tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        subtitle = hb_list_item( title->list_subtitle, i );
        fprintf( stderr, "    + %d, %s (iso639-2: %s)\n", i + 1, subtitle->lang,
            subtitle->iso639_2);
    }
}

static int HandleEvents( hb_handle_t * h )
{
    hb_state_t s;
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

            if( chapter_start && chapter_end )
            {
                job->chapter_start = MAX( job->chapter_start,
                                          chapter_start );
                job->chapter_end   = MIN( job->chapter_end,
                                          chapter_end );
                job->chapter_end   = MAX( job->chapter_start,
                                          job->chapter_end );
            }

            if (preset)
            {
                fprintf( stderr, "+ Using preset: %s", preset_name);
                
                if (!strcmp(preset_name, "Animation"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1000;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    x264opts = strdup("ref=5:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=5:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip:filter=2,2");
                    deinterlace = 1;
                    deinterlace_opt = "2:-1:1";
                    job->chapter_markers = 1;
                    pixelratio = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "AppleTV"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 2500;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    x264opts = strdup("bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:trellis=1:cabac=0");
                    job->chapter_markers = 1;
                    pixelratio = 1;
                }

                if (!strcmp(preset_name, "Bedlam"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1800;
                    acodec = HB_ACODEC_AC3;
                    x264opts = strdup("ref=16:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=7:me-range=64:analyse=all:8x8dct:trellis=2:no-fast-pskip:no-dct-decimate:filter=-2,-1");
                    job->chapter_markers = 1;
                    pixelratio = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "Blind"))
                {
                    mux = HB_MUX_MP4;
                    job->vbitrate = 512;
                    job->abitrate = 128;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    job->width = 512;
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "Broke"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    size = 695;
                    job->abitrate = 128;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    job->width = 640;
                    x264opts = strdup("ref=3:mixed-refs:bframes=16:bime:weightb:b-rdo:b-pyramid:direct=auto:me=umh:subme=6:trellis=1:analyse=all:8x8dct:no-fast-pskip");
                    job->chapter_markers = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "Classic"))
                {
                    mux = HB_MUX_MP4;
                    job->vbitrate = 1000;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                }

                if (!strcmp(preset_name, "Constant Quality Rate"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vquality = 0.64709997177124023;
                    job->crf = 1;
                    acodec = HB_ACODEC_AC3;
                    x264opts = strdup("ref=3:mixed-refs:bframes=3:b-pyramid:b-rdo:bime:weightb:filter=-2,-1:subme=6:trellis=1:analyse=all:8x8dct:me=umh");
                    job->chapter_markers = 1;
                    pixelratio = 1;
                }

                if (!strcmp(preset_name, "Deux Six Quatre"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1600;
                    acodec = HB_ACODEC_AC3;
                    x264opts = strdup("ref=5:mixed-refs:bframes=3:bime:weightb:b-rdo:b-pyramid:me=umh:subme=7:trellis=1:analyse=all:8x8dct:no-fast-pskip");
                    job->chapter_markers = 1;
                    pixelratio = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "Film"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1800;
                    acodec = HB_ACODEC_AC3;
                    x264opts = strdup("ref=3:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=7:analyse=all:8x8dct:trellis=1:no-fast-pskip");
                    job->chapter_markers = 1;
                    pixelratio = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "iPhone / iPod Touch"))
                {
                    mux = HB_MUX_MP4;
                    job->ipod_atom = 1;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 960;
                    job->abitrate = 128;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    job->width = 480;
                    x264opts = strdup("level=30:cabac=0:ref=1:analyse=all:me=umh:subme=6:no-fast-pskip=1:trellis=1");
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "iPod High-Rez"))
                {
                    mux = HB_MUX_MP4;
                    job->ipod_atom = 1;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1500;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    job->width = 640;
                    x264opts = strdup("level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:subme=6:no-fast-pskip=1");
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "iPod Low-Rez"))
                {
                    mux = HB_MUX_MP4;
                    job->ipod_atom = 1;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 700;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    job->width = 320;
                    x264opts = strdup("level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:subme=6:no-fast-pskip=1");
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "Normal"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1500;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    x264opts = strdup("ref=2:bframes=2:subme=5:me=umh");
                    job->chapter_markers = 1;
                    pixelratio = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "PS3"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 2500;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    x264opts = strdup("level=41:subme=5:me=umh");
                    pixelratio = 1;
                }

                if (!strcmp(preset_name, "PSP"))
                {
                    mux = HB_MUX_MP4;
                    job->vbitrate = 1024;
                    job->abitrate = 128;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    job->width = 368;
                    job->height = 208;
                    job->chapter_markers = 1;
                }

                if (!strcmp(preset_name, "QuickTime"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 2000;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    x264opts = strdup("ref=3:mixed-refs:bframes=3:bime:weightb:b-rdo:direct=auto:me=umh:subme=5:analyse=all:trellis=1:no-fast-pskip");
                    job->chapter_markers = 1;
                    pixelratio = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "Television"))
                {
                    mux = HB_MUX_MKV;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 1300;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    x264opts = strdup("ref=3:mixed-refs:bframes=16:bime:weightb:direct=auto:b-pyramid:me=umh:subme=6:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip");
                    deinterlace = 1;
                    deinterlace_opt = "2:-1:1";
                    denoise = 1;
                    denoise_opt = "2:1:2:3";
                    job->chapter_markers = 1;
                    twoPass = 1;
                    turbo_opts_enabled = 1;
                }

                if (!strcmp(preset_name, "Xbox 360"))
                {
                    mux = HB_MUX_MP4;
                    vcodec = HB_VCODEC_X264;
                    job->vbitrate = 2000;
                    job->abitrate = 160;
                    job->arate = 48000;
                    acodec = HB_ACODEC_FAAC;
                    x264opts = strdup("level=40:ref=3:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=7:analyse=all:8x8dct:trellis=1:no-fast-pskip:filter=-2,-1");
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
                job->pixel_ratio = 2;
                if (modulus)
                {
                    job->modulus = modulus;
                }
            }
            else
            {
                job->pixel_ratio = pixelratio;
            }
            /* Add selected filters */
            job->filters = hb_list_init();
            if( detelecine )
            {
                hb_filter_detelecine.settings = detelecine_opt;
                hb_list_add( job->filters, &hb_filter_detelecine );
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
            
            if( vquality >= 0.0 && vquality <= 1.0 )
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
                job->vrate = 27000000;
                job->vrate_base = vrate;
            }
            if( arate )
            {
                job->arate = arate;
            }

            if( audios )
            {
                if( strcasecmp( audios, "none" ) )
                {
                    int    audio_count = 0;
                    char * tmp         = audios;
                    while( *tmp )
                    {
                        if( *tmp < '0' || *tmp > '9' )
                        {
                            /* Skip non numeric char */
                            tmp++;
                            continue;
                        }
						job->audio_mixdowns[audio_count] = audio_mixdown;
                        job->audios[audio_count++] =
                            strtol( tmp, &tmp, 0 ) - 1;
                    }
                    job->audios[audio_count] = -1;
                }
                else
                {
                    job->audios[0] = -1;
                }
            }
			else
			{
			    /* default to the first audio track if none has been specified */
			    job->audios[0] = 0;
			    job->audio_mixdowns[0] = audio_mixdown;
			}
			
			if( audio_mixdown == HB_AMIXDOWN_DOLBYPLII_AC3)
			{
			    
			    
               int i;
               for( i = 3 ; i > 0; i--)
               {                   
                   job->audios[i*2+1] = job->audios[i];
                   job->audios[i*2] = job->audios[i];
                   if(job->audios[i] != -1  )
                   {
                       job->audio_mixdowns[i*2+1] = HB_AMIXDOWN_AC3;
                       job->audio_mixdowns[i*2] = HB_AMIXDOWN_DOLBYPLII;
                   }
               }
               
               job->audios[1] = job->audios[0];
               job->audio_mixdowns[1] = HB_AMIXDOWN_AC3;
               job->audio_mixdowns[0] = HB_AMIXDOWN_DOLBYPLII;
            }
			
            if( abitrate )
            {
                job->abitrate = abitrate;
            }
            if( acodec )
            {
                job->acodec = acodec;
            }
            if ( dynamic_range_compression )
            {
                job->dynamic_range_compression = dynamic_range_compression;
            }
            
            if( size )
            {
                job->vbitrate = hb_calc_bitrate( job, size );
                fprintf( stderr, "Calculated bitrate: %d kbps\n",
                         job->vbitrate );
            }

            if( sub )
            {
                job->subtitle = sub - 1;
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
            
            if (vfr)
                job->vfr = 1;
	
            if( subtitle_force )
            {
                job->subtitle_force = subtitle_force;
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
            fprintf( stdout, "\rMuxing: %.2f %%", 100.0 * p.progress );
            fflush(stdout);
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
    
    fprintf( stderr,
    "Syntax: HandBrakeCLI [options] -i <device> -o <file>\n"
    "\n"
	"### General Handbrake Options------------------------------------------------\n\n"
    "    -h, --help              Print help\n"
    "    -u, --update            Check for updates and exit\n"
    "    -v, --verbose           Be verbose\n"
    "    -C, --cpu               Set CPU count (default: autodetected)\n"
    "    -Z. --preset <string>   Use a built-in preset. Capitalization matters, and\n"
    "                            if the preset name has spaces, surround it with\n"
    "                            double quotation marks\n"
    "    -z, --preset-list       See a list of available built-in presets\n"
    "\n"
	
	"### Source Options-----------------------------------------------------------\n\n"
	"    -i, --input <string>    Set input device\n"
	"    -t, --title <number>    Select a title to encode (0 to scan only,\n"
    "                            default: 1)\n"
    "    -L, --longest           Select the longest title\n"
    "    -c, --chapters <string> Select chapters (e.g. \"1-3\" for chapters\n"
    "                            1 to 3, or \"3\" for chapter 3 only,\n"
    "                            default: all chapters)\n"
	"\n"
	
	"### Destination Options------------------------------------------------------\n\n"
    "    -o, --output <string>   Set output file name\n"
	"    -f, --format <string>   Set output format (avi/mp4/ogm/mkv, default:\n"
    "                            autodetected from file name)\n"
    "    -4, --large-file        Use 64-bit mp4 files that can hold more than\n"
    "                            4 GB. Note: Breaks iPod, @TV, PS3 compatibility.\n"""
    "    -O, --optimize          Optimize mp4 files for HTTP streaming\n"
    "    -I, --ipod-atom         Mark mp4 files so iPods will accept them\n"
    "\n"
	
	"### Picture Settings---------------------------------------------------------\n\n"
    "    -w, --width <number>    Set picture width\n"
    "    -l, --height <number>   Set picture height\n"
    "        --crop <T:B:L:R>    Set cropping values (default: autocrop)\n"
	"    -Y, --maxHeight <#>     Set maximum height\n"
	"    -X, --maxWidth <#>      Set maximum width\n"
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
	"    -m, --markers           Add chapter markers (mp4 output format only)\n"
	"\n"
	
	"### Video Options------------------------------------------------------------\n\n"
	"    -e, --encoder <string>  Set video library encoder (ffmpeg,xvid,\n"
    "                            x264,x264b13,x264b30 default: ffmpeg)\n"
	"    -q, --quality <float>   Set video quality (0.0..1.0)\n"
	"    -Q, --cqp               Use with -q for CQP instead of CRF\n"
    "    -S, --size <MB>         Set target size\n"
	"    -b, --vb <kb/s>         Set video bitrate (default: 1000)\n"
	"    -r, --rate              Set video framerate (" );
    for( i = 0; i < hb_video_rates_count; i++ )
    {
        fprintf( stderr, hb_video_rates[i].string );
        if( i != hb_video_rates_count - 1 )
            fprintf( stderr, "/" );
    }
    fprintf( stderr, ")\n"
	"\n"
	"    -2, --two-pass          Use two-pass mode\n"
     "    -d, --deinterlace       Deinterlace video with yadif/mcdeint filter\n"
     "          <YM:FD:MM:QP>     (default 0:-1:-1:1)\n"
     "           or\n"
     "          <fast/slow/slower/slowest>\n"            
     "    -7, --deblock           Deblock video with pp7 filter\n"
     "          <QP:M>            (default 0:2)\n"
     "    -8, --denoise           Denoise video with hqdn3d filter\n"
     "          <SL:SC:TL:TC>     (default 4:3:6:4.5)\n"
     "           or\n"
     "          <weak/medium/strong>\n"
     "    -9, --detelecine        Detelecine video with pullup filter\n"
     "          <L:R:T:B:SB:MP>   (default 1:1:4:4:0:0)\n"
    "    -g, --grayscale         Grayscale encoding\n"
    "    -p, --pixelratio        Store pixel aspect ratio in video stream\n"
    "    -P, --loosePixelratio   Store pixel aspect ratio with specified width\n"
    "           <modulus>        Takes as optional argument what number you want\n"
    "                            the dimensions to divide cleanly by (default 16)\n"
    
	
	"\n"
	
	
	"### Audio Options-----------------------------------------------------------\n\n"
	"    -E, --aencoder <string> Audio encoder (faac/lame/vorbis/ac3/aac+ac3) \n"
	"                            ac3 meaning passthrough, ac3+aac meaning an\n"
	"                            aac dpl2 mixdown paired with ac3 pass-thru\n"
	"                            (default: guessed)\n"
	"    -B, --ab <kb/s>         Set audio bitrate (default: 128)\n"
	"    -a, --audio <string>    Select audio channel(s), separated by commas\n"
	"                            (\"none\" for no audio, \"1,2,3\" for multiple\n"
	"                             tracks, default: first one,\n"
	"                             max 8 normally, max 4 with aac+ac3)\n"
    "    -6, --mixdown <string>  Format for surround sound downmixing\n"
    "                            (mono/stereo/dpl1/dpl2/6ch, default: dpl2)\n"
    "    -R, --arate             Set audio samplerate (" );
    for( i = 0; i < hb_audio_rates_count; i++ )
    {
        fprintf( stderr, hb_audio_rates[i].string );
        if( i != hb_audio_rates_count - 1 )
            fprintf( stderr, "/" );
    }
    fprintf( stderr, " kHz)\n"
    "    -D, --drc <float>       Apply extra dynamic range compression to the audio,\n"
    "                            making soft sounds louder. Range is 1.0 to 4.0\n"
    "                            (too loud), with 1.5 - 2.5 being a useful range.\n"
    
	
	"\n"
	
	
    "### Advanced Options---------------------------------------------------------\n\n"
    "    -x, --x264opts <string> Specify advanced x264 options in the\n"
    "                            same style as mencoder:\n"
    "                            option1=value1:option2=value2\n"
    "    -T, --turbo             When using 2-pass use the turbo options\n"
    "                            on the first pass to improve speed\n"
    "                            (only works with x264, affects PSNR by about 0.05dB,\n"
    "                            and increases first pass speed two to four times)\n"
    "    -V, --vfr               Perform variable framerate detelecine on NTSC video\n"
    );
}

/****************************************************************************
 * ShowPresets:
 ****************************************************************************/
static void ShowPresets()
{
    printf("\n+ Animation:  -e x264 -b 1000 -B 160 -R 48 -E faac -f mkv --deinterlace=\"slower\" -m -p -2 -T -x ref=5:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=5:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip:filter=2,2\n");

    printf("\n+ AppleTV:  -e x264 -b 2500 -B 160 -R 48 -E faac -f mp4 -m -p -x bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:trellis=1:cabac=0\n");

    printf("\n+ Bedlam:  -e x264 -b 1800 -E ac3 -f mkv -m -p -2 -T -x ref=16:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=7:me-range=64:analyse=all:8x8dct:trellis=2:no-fast-pskip:no-dct-decimate:filter=-2,-1\n");

    printf("\n+ Blind:  -b 512 -B 128 -R 48 -E faac -f mp4 -w 512 -m\n");

    printf("\n+ Broke:  -e x264 -S 695 -B 128 -R 48 -E faac -f mp4 -w 640 -m -2 -T -x ref=3:mixed-refs:bframes=16:bime:weightb:b-rdo:b-pyramid:direct=auto:me=umh:subme=6:trellis=1:analyse=all:8x8dct:no-fast-pskip\n");

    printf("\n+ Classic:  -b 1000 -B 160 -R 48 -E faac -f mp4\n");

    printf("\n+ Constant Quality Rate:  -e x264 -q 0.64709997177124023 -Q  -E ac3 -f mkv -m -p -x ref=3:mixed-refs:bframes=3:b-pyramid:b-rdo:bime:weightb:filter=-2,-1:subme=6:trellis=1:analyse=all:8x8dct:me=umh\n");

    printf("\n+ Deux Six Quatre:  -e x264 -b 1600 -E ac3 -f mkv -m -p -2 -T -x ref=5:mixed-refs:bframes=3:bime:weightb:b-rdo:b-pyramid:me=umh:subme=7:trellis=1:analyse=all:8x8dct:no-fast-pskip\n");

    printf("\n+ Film:  -e x264 -b 1800 -E ac3 -f mkv -m -p -2 -T -x ref=3:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=7:analyse=all:8x8dct:trellis=1:no-fast-pskip\n");

    printf("\n+ iPhone / iPod Touch:  -e x264 -b 960 -B 128 -R 48 -E faac -f mp4 -I -w 480 -m -x level=30:cabac=0:ref=1:analyse=all:me=umh:subme=6:no-fast-pskip=1:trellis=1\n");

    printf("\n+ iPod High-Rez:  -e x264 -b 1500 -B 160 -R 48 -E faac -f mp4 -I -w 640 -m -x level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:subme=6:no-fast-pskip=1\n");

    printf("\n+ iPod Low-Rez:  -e x264 -b 700 -B 160 -R 48 -E faac -f mp4 -I -w 320 -m -x level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:subme=6:no-fast-pskip=1\n");

    printf("\n+ Normal:  -e x264 -b 1500 -B 160 -R 48 -E faac -f mp4 -m -p -2 -T -x ref=2:bframes=2:subme=5:me=umh\n");

    printf("\n+ PS3:  -e x264 -b 2500 -B 160 -R 48 -E faac -f mp4 -p -x level=41:subme=5:me=umh\n");

    printf("\n+ PSP:  -b 1024 -B 128 -R 48 -E faac -f mp4 -w 368 -l 208 -m\n");

    printf("\n+ QuickTime:  -e x264 -b 2000 -B 160 -R 48 -E faac -f mp4 -m -p -2 -T -x ref=3:mixed-refs:bframes=3:bime:weightb:b-rdo:direct=auto:me=umh:subme=5:analyse=all:trellis=1:no-fast-pskip\n");

    printf("\n+ Television:  -e x264 -b 1300 -B 160 -R 48 -E faac -f mkv --deinterlace=\"slower\" --denoise=\"weak\" -m -2 -T -x ref=3:mixed-refs:bframes=16:bime:weightb:direct=auto:b-pyramid:me=umh:subme=6:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip\n");

    printf("\n+ Xbox 360:  -e x264 -b 2000 -B 160 -R 48 -E faac -f mp4 -p -x level=40:ref=3:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=7:analyse=all:8x8dct:trellis=1:no-fast-pskip:filter=-2,-1\n");
}

/****************************************************************************
 * ParseOptions:
 ****************************************************************************/
static int ParseOptions( int argc, char ** argv )
{
    for( ;; )
    {
        static struct option long_options[] =
          {
            { "help",        no_argument,       NULL,    'h' },
            { "update",      no_argument,       NULL,    'u' },
            { "verbose",     no_argument,       NULL,    'v' },
            { "cpu",         required_argument, NULL,    'C' },

            { "format",      required_argument, NULL,    'f' },
            { "input",       required_argument, NULL,    'i' },
            { "output",      required_argument, NULL,    'o' },
            { "large-file",  no_argument,       NULL,    '4' },
            { "optimize",    no_argument,       NULL,    'O' },
            { "ipod-atom",   no_argument,       NULL,    'I' },
            
            { "title",       required_argument, NULL,    't' },
            { "longest",     no_argument,       NULL,    'L' },
            { "chapters",    required_argument, NULL,    'c' },
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
            { "vfr",         no_argument,       NULL,    'V' },
			
            { 0, 0, 0, 0 }
          };

        int option_index = 0;
        int c;

		c = getopt_long( argc, argv,
						 "hvuC:f:4i:Io:t:Lc:m::a:6:s:UFN:e:E:2dD:789gpOP::w:l:n:b:q:S:B:r:R:Qx:TY:X:VZ:z",
                         long_options, &option_index );
        if( c < 0 )
        {
            break;
        }

        switch( c )
        {
            case 'h':
                ShowHelp();
                exit( 0 );
            case 'u':
                update = 1;
                break;
            case 'v':
                debug = HB_DEBUG_ALL;
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
                
            case 'f':
                format = strdup( optarg );
                break;
            case 'i':
                input = strdup( optarg );
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
            case 'm':
                if( optarg != NULL )
                {
                    marker_file = strdup( optarg );
                }
                chapter_markers = 1;
                break;
            case 'a':
                audios = strdup( optarg );
                break;
            case '6':
                if( !strcasecmp( optarg, "mono" ) )
                {
                    audio_mixdown = HB_AMIXDOWN_MONO;
                }
                else if( !strcasecmp( optarg, "stereo" ) )
                {
                    audio_mixdown = HB_AMIXDOWN_STEREO;
                }
                else if( !strcasecmp( optarg, "dpl1" ) )
                {
                    audio_mixdown = HB_AMIXDOWN_DOLBY;
                }
                else if( !strcasecmp( optarg, "dpl2" ) )
                {
                    audio_mixdown = HB_AMIXDOWN_DOLBYPLII;
                }
                else if( !strcasecmp( optarg, "6ch" ) )
                {
                    audio_mixdown = HB_AMIXDOWN_6CH;
                }
                break;
            case 'D':
                dynamic_range_compression = atof( optarg );
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
                        deinterlace_opt = "0";
                    }
                    else if (!( strcmp( optarg, "slower" ) ))
                    {
                        deinterlace_opt = "2:-1:1";
                    }
                    else if (!( strcmp( optarg, "slowest" ) ))
                    {
                        deinterlace_opt = "1:-1:1";
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
                    modulus = atoi( optarg );
                }
                break;
            case 'e':
                if( !strcasecmp( optarg, "ffmpeg" ) )
                {
                    vcodec = HB_VCODEC_FFMPEG;
                }
                else if( !strcasecmp( optarg, "xvid" ) )
                {
                    vcodec = HB_VCODEC_XVID;
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
                else
                {
                    fprintf( stderr, "invalid codec (%s)\n", optarg );
                    return -1;
                }
                break;
            case 'E':
                if( !strcasecmp( optarg, "ac3" ) )
                {
                    acodec = HB_ACODEC_AC3;
                }
                else if( !strcasecmp( optarg, "lame" ) )
                {
                    acodec = HB_ACODEC_LAME;
                }
                else if( !strcasecmp( optarg, "faac" ) )
                {
                    acodec = HB_ACODEC_FAAC;
                }
                else if( !strcasecmp( optarg, "vorbis") )
                {
                    acodec = HB_ACODEC_VORBIS;
                }
                else if( !strcasecmp( optarg, "aac+ac3") )
                {
                    acodec = HB_ACODEC_FAAC;
                    audio_mixdown = HB_AMIXDOWN_DOLBYPLII_AC3;
                    arate = 48000;
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
                break;
            }
            case 'R':
            {
                int i;
                arate = 0;
                for( i = 0; i < hb_audio_rates_count; i++ )
                {
                    if( !strcmp( optarg, hb_audio_rates[i].string ) )
                    {
                        arate = hb_audio_rates[i].rate;
                        break;
                    }
                }
                if( !arate )
                {
                    fprintf( stderr, "invalid framerate %s\n", optarg );
                }
                break;
            }
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
                abitrate = atoi( optarg );
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
            case 'V':
                vfr = 1;
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
            }
            else if( p && ( !strcasecmp( p, ".mp4" )  ||
                            !strcasecmp( p, ".m4v" ) ) )
            {
                if ( h264_30 == 1 )
                    mux = HB_MUX_IPOD;
                else
                    mux = HB_MUX_MP4;
            }
            else if( p && ( !strcasecmp( p, ".ogm" ) ||
                            !strcasecmp( p, ".ogg" ) ) )
            {
                mux = HB_MUX_OGM;
            }
            else if( p && !strcasecmp(p, ".mkv" ) )
            {
                mux = HB_MUX_MKV;
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
        }
        else if( !strcasecmp( format, "mp4" ) )
        {
            if ( h264_30 == 1)
                mux = HB_MUX_IPOD;
            else
                mux = HB_MUX_MP4;
        }
        else if( !strcasecmp( format, "ogm" ) ||
                 !strcasecmp( format, "ogg" ) )
        {
            mux = HB_MUX_OGM;
        }
        else if( !strcasecmp( format, "mkv" ) )
        {
            mux = HB_MUX_MKV;
        }
        else
        {
            fprintf( stderr, "Invalid output format (%s). Possible "
                     "choices are avi, mp4, m4v, ogm, ogg and mkv\n.", format );
            return 1;
        }

        if( !acodec )
        {
            if( mux == HB_MUX_MP4 || mux == HB_MUX_IPOD )
            {
                acodec = HB_ACODEC_FAAC;
            }
            else if( mux == HB_MUX_AVI )
            {
                acodec = HB_ACODEC_LAME;
            }
            else if( mux == HB_MUX_OGM )
            {
                acodec = HB_ACODEC_VORBIS;
            }
            else if( mux == HB_MUX_MKV )
            {
                acodec = HB_ACODEC_AC3;
            }
        }

    }

    return 0;
}
