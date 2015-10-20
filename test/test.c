/* test.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#if defined( __MINGW32__ )
#include <windows.h>
#include <conio.h>
#endif

#if defined( PTW32_STATIC_LIB )
#include <pthread.h>
#endif

#include "hb.h"
#include "lang.h"
#include "parsecsv.h"
#include "openclwrapper.h"

#ifdef USE_QSV
#include "qsv_common.h"
#endif

#if defined( __APPLE_CC__ )
#import <CoreServices/CoreServices.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#endif

/* Options */
static int     debug               = HB_DEBUG_ALL;
static int     update              = 0;
static int     dvdnav              = 1;
static char *  input               = NULL;
static char *  output              = NULL;
static char *  format              = NULL;
static int     titleindex          = 1;
static int     titlescan           = 0;
static int     main_feature        = 0;
static char *  native_language     = NULL;
static int     native_dub          = 0;
static int     twoPass             = 0;
static int     deinterlace_disable = 0;
static int     deinterlace_custom  = 0;
static char *  deinterlace         = NULL;
static int     deblock_disable     = 0;
static char *  deblock             = NULL;
static int     hqdn3d_disable      = 0;
static int     hqdn3d_custom       = 0;
static char *  hqdn3d              = NULL;
static int     nlmeans_disable     = 0;
static int     nlmeans_custom      = 0;
static char *  nlmeans             = NULL;
static char *  nlmeans_tune        = NULL;
static int     detelecine_disable  = 0;
static int     detelecine_custom   = 0;
static char *  detelecine          = NULL;
static int     decomb_disable      = 0;
static int     decomb_custom       = 0;
static char *  decomb              = NULL;
static char *  rotate              = NULL;
static int     grayscale           = -1;
static char *  vcodec              = NULL;
static int     audio_all                 = -1;
static char ** audio_copy_list           = NULL;
static char ** audio_lang_list           = NULL;
static char ** atracks                   = NULL;
static char ** acodecs                   = NULL;
static char ** abitrates                 = NULL;
static char ** aqualities                = NULL;
static char ** arates                    = NULL;
static char ** mixdowns                  = NULL;
static char ** normalize_mix_level       = NULL;
static char ** audio_dither              = NULL;
static char ** dynamic_range_compression = NULL;
static char ** audio_gain                = NULL;
static char ** acompressions             = NULL;
static char *  acodec_fallback           = NULL;
static char ** anames                    = NULL;
static char ** subtitle_lang_list        = NULL;
static char ** subtracks                 = NULL;
static char ** subforce                  = NULL;
static int     subtitle_all              = -1;
static int     subburn                   = 0;
static int     subburn_native            = 0;
static int     subdefault                = 0;
static char ** srtfile                   = NULL;
static char ** srtcodeset                = NULL;
static char ** srtoffset                 = NULL;
static char ** srtlang                   = NULL;
static int     srtdefault                = -1;
static int     srtburn                   = -1;
static int      width       = 0;
static int      height      = 0;
static int      crop[4]     = { -1,-1,-1,-1 };
static int      loose_crop  = -1;
static char *   vrate       = NULL;
static float    vquality    = -1.0;
static int      vbitrate    = 0;
static int      mux         = 0;
static int      anamorphic_mode     = -1;
static int      modulus             = 0;
static int      par_height          = -1;
static int      par_width           = -1;
static int      display_width       = -1;
static int      keep_display_aspect = 0;
static int      itu_par             = -1;
static int      angle               = 0;
static int      chapter_start       = 0;
static int      chapter_end         = 0;
static int      chapter_markers     = -1;
static char *   marker_file         = NULL;
static char *   encoder_preset  = NULL;
static char *   encoder_tune    = NULL;
static char *   encoder_profile = NULL;
static char *   encoder_level   = NULL;
static char *   advanced_opts   = NULL;
static int      maxHeight     = 0;
static int      maxWidth      = 0;
static int      fastfirstpass = 0;
static char *   preset_export_name   = NULL;
static char *   preset_export_desc   = NULL;
static char *   preset_export_file   = NULL;
static char *   preset_name        = NULL;
static int      cfr           = -1;
static int      mp4_optimize  = -1;
static int      ipod_atom     = -1;
static int      color_matrix_code = -1;
static int      preview_count = 10;
static int      store_previews = 0;
static int      start_at_preview = 0;
static int64_t  start_at_pts    = 0;
static int      start_at_frame = 0;
static int64_t  stop_at_pts    = 0;
static int      stop_at_frame = 0;
static uint64_t min_title_duration = 10;
static int      use_opencl         = -1;
static int      use_hwd            = -1;
#ifdef USE_QSV
static int      qsv_async_depth    = -1;
static int      qsv_decode         = -1;
#endif

/* Exit cleanly on Ctrl-C */
static volatile hb_error_code done_error = HB_ERROR_NONE;
static volatile int die = 0;
static void SigHandler( int );

/* Utils */
static void ShowHelp();
static void ShowCommands()
{
    fprintf(stdout, "\nCommands:\n");
    fprintf(stdout, " [h]elp    Show this message\n");
    fprintf(stdout, " [q]uit    Exit HandBrakeCLI\n");
    fprintf(stdout, " [p]ause   Pause encoding\n");
    fprintf(stdout, " [r]esume  Resume encoding\n");
}

static int         ParseOptions( int argc, char ** argv );
static int         CheckOptions( int argc, char ** argv );
static int         HandleEvents( hb_handle_t * h, hb_dict_t *preset_dict );
static hb_dict_t * PreparePreset( const char *preset_name );
static hb_dict_t * PrepareJob( hb_handle_t *h, hb_title_t *title,
                               hb_dict_t *preset_dict );

static int str_vlen(char **strv);
static void str_vfree( char **strv );
static char** str_split( char *str, char delem );

static void print_string_list(FILE *out, const char* const *list, const char *prefix);

#ifdef __APPLE_CC__
static char* bsd_name_for_path(char *path);
static int device_is_dvd(char *device);
static io_service_t get_iokit_service( char *device );
static int is_dvd_service( io_service_t service );
static int is_whole_media_service( io_service_t service );
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

static int get_argv_utf8(int *argc_ptr, char ***argv_ptr)
{
#if defined( __MINGW32__ )
    int ret = 0;
    int argc;
    char **argv;

    wchar_t **argv_utf16 = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv_utf16)
    {
        int i;
        int offset = (argc+1) * sizeof(char*);
        int size = offset;

        for(i = 0; i < argc; i++)
            size += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, NULL, 0, NULL, NULL );

        argv = malloc(size);
        if (argv != NULL)
        {
            for (i = 0; i < argc; i++)
            {
                argv[i] = (char*)argv + offset;
                offset += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, argv[i], size-offset, NULL, NULL);
            }
            argv[argc] = NULL;
            ret = 1;
        }
        LocalFree(argv_utf16);
    }
    if (ret)
    {
        *argc_ptr = argc;
        *argv_ptr = argv;
    }
    return ret;
#else
    // On other systems, assume command line is already utf8
    return 1;
#endif
}

int main( int argc, char ** argv )
{
    hb_handle_t * h;
    int           build;
    char        * version;

    hb_global_init();
    hb_presets_builtin_update();

    /* Init libhb */
    h = hb_init(4, 0);  // Show all logging until debug level is parsed

    // Get utf8 command line if windows
    get_argv_utf8(&argc, &argv);

    /* Parse command line */
    if( ParseOptions( argc, argv ) ||
        CheckOptions( argc, argv ) )
    {
        return 1;
    }

    hb_log_level_set(h, debug);

    /* Register our error handler */
    hb_register_error_handler(&hb_cli_error_handler);

    hb_dvd_set_dvdnav( dvdnav );

    /* Show version */
    fprintf( stderr, "%s - %s - %s\n",
             HB_PROJECT_TITLE, HB_PROJECT_BUILD_TITLE, HB_PROJECT_URL_WEBSITE );

    /* Check for update */
    if( update )
    {
        hb_update_poll(h);
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
        hb_global_close();
        return 0;
    }

    /* Geeky */
    fprintf( stderr, "%d CPU%s detected\n", hb_get_cpu_count(),
             hb_get_cpu_count() > 1 ? "s" : "" );

    /* Exit ASAP on Ctrl-C */
    signal( SIGINT, SigHandler );

    // Apply all command line overrides to the preset that are possible.
    // Some command line options are applied later to the job
    // (e.g. chapter names, explicit audio & subtitle tracks).
    hb_dict_t *preset_dict = PreparePreset(preset_name);
    if (preset_dict == NULL)
    {
        // An appropriate error message should have already
        // been spilled by PreparePreset.
        return 1;
    }

    if (preset_export_name != NULL)
    {
        hb_dict_set(preset_dict, "PresetName",
                    hb_value_string(preset_export_name));
        if (preset_export_desc != NULL)
        {
            hb_dict_set(preset_dict, "PresetDescription",
                        hb_value_string(preset_export_desc));
        }
        if (preset_export_file != NULL)
        {
            hb_presets_write_json(preset_dict, preset_export_file);
        }
        else
        {
            char *json;
            json = hb_presets_package_json(preset_dict);
            fprintf(stdout, "%s\n", json);
        }
        // If the user requested to export a preset, but not to
        // transcode or scan a file, exit here.
        if (input == NULL || (!titlescan && titleindex != 0 && output == NULL))
        {
            hb_value_free(&preset_dict);
            return 0;
        }
    }

    /* Feed libhb with a DVD to scan */
    fprintf( stderr, "Opening %s...\n", input );

    if (main_feature) {
        /*
         * We need to scan for all the titles in order to find the main feature
         */
        titleindex = 0;
    }

    hb_system_sleep_prevent(h);

    // FIXME: When hardware decode is enabled, the scan must be performed
    // with hardware decode enabled because the decoder context used during
    // encoding phase comes from the context used during scan.  This is
    // broken by design and I would very much like to fix this someday.
    hb_hwd_set_enable(h, hb_value_get_bool(
                         hb_dict_get(preset_dict, "VideoHWDecode")));
    hb_scan(h, input, titleindex, preview_count, store_previews,
            min_title_duration * 90000LL);

    /* Wait... */
    while( !die )
    {
#if defined( __MINGW32__ )
        if( _kbhit() ) {
            switch( _getch() )
            {
                case 0x03: /* ctrl-c */
                case 'q':
                    fprintf( stdout, "\nEncoding Quit by user command\n" );
                    done_error = HB_ERROR_CANCELED;
                    die = 1;
                    break;
                case 'p':
                    fprintf(stdout,
                            "\nEncoding Paused by user command, 'r' to resume\n");
                    hb_pause(h);
                    hb_system_sleep_allow(h);
                    break;
                case 'r':
                    hb_system_sleep_prevent(h);
                    hb_resume(h);
                    break;
                case 'h':
                    ShowCommands();
                    break;
            }
        }
        hb_snooze( 200 );
#elif !defined(SYS_BEOS)
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
                        done_error = HB_ERROR_CANCELED;
                        die = 1;
                        break;
                    case 'p':
                        fprintf(stdout,
                                "\nEncoding Paused by user command, 'r' to resume\n");
                        hb_pause(h);
                        hb_system_sleep_allow(h);
                        break;
                    case 'r':
                        hb_system_sleep_prevent(h);
                        hb_resume(h);
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

        HandleEvents( h, preset_dict );
    }

    /* Clean up */
    hb_value_free(&preset_dict);
    hb_close(&h);
    hb_global_close();
    str_vfree(audio_copy_list);
    str_vfree(abitrates);
    str_vfree(acompressions);
    str_vfree(aqualities);
    str_vfree(audio_dither);
    str_vfree(acodecs);
    str_vfree(arates);
    str_vfree(atracks);
    str_vfree(audio_lang_list);
    str_vfree(audio_gain);
    str_vfree(dynamic_range_compression);
    str_vfree(mixdowns);
    str_vfree(subtitle_lang_list);
    str_vfree(subtracks);
    free(acodec_fallback);
    free(native_language);
    free(format);
    free(input);
    free(output);
    free(preset_name);
    free(encoder_preset);
    free(encoder_tune);
    free(advanced_opts);
    free(encoder_profile);
    free(encoder_level);
    free(rotate);
    free(deblock);
    free(detelecine);
    free(deinterlace);
    free(decomb);
    free(hqdn3d);
    free(nlmeans);
    free(nlmeans_tune);
    free(preset_export_name);
    free(preset_export_desc);
    free(preset_export_file);

    // write a carriage return to stdout
    // avoids overlap / line wrapping when stderr is redirected
    fprintf(stdout, "\n");
    fprintf(stderr, "HandBrake has exited.\n");

    return done_error;
}

static void PrintTitleInfo( hb_title_t * title, int feature )
{
    int i;

    fprintf( stderr, "+ title %d:\n", title->index );
    if ( title->index == feature )
    {
        fprintf( stderr, "  + Main Feature\n" );
    }
    if ( title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE )
    {
        fprintf( stderr, "  + stream: %s\n", title->path );
    }
    else if ( title->type == HB_DVD_TYPE )
    {
        fprintf( stderr, "  + vts %d, ttn %d, cells %d->%d (%"PRIu64" blocks)\n",
                title->vts, title->ttn, title->cell_start, title->cell_end,
                title->block_count );
    }
    else if( title->type == HB_BD_TYPE )
    {
        fprintf( stderr, "  + playlist: %05d.MPLS\n", title->playlist );
    }
    if (title->angle_count > 1)
        fprintf( stderr, "  + angle(s) %d\n", title->angle_count );
    fprintf( stderr, "  + duration: %02d:%02d:%02d\n",
             title->hours, title->minutes, title->seconds );
    fprintf( stderr, "  + size: %dx%d, pixel aspect: %d/%d, display aspect: %.2f, %.3f fps\n",
             title->geometry.width, title->geometry.height,
             title->geometry.par.num, title->geometry.par.den,
             (float)title->dar.num / title->dar.den,
             (float)title->vrate.num / title->vrate.den );
    fprintf( stderr, "  + autocrop: %d/%d/%d/%d\n", title->crop[0],
             title->crop[1], title->crop[2], title->crop[3] );

    fprintf(stderr, "  + support opencl: %s\n", title->opencl_support ? "yes" : "no");

    fprintf( stderr, "  + chapters:\n" );
    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        hb_chapter_t  * chapter;
        chapter = hb_list_item( title->list_chapter, i );
        fprintf( stderr, "    + %d: cells %d->%d, %"PRIu64" blocks, duration "
                 "%02d:%02d:%02d\n", chapter->index,
                 chapter->cell_start, chapter->cell_end,
                 chapter->block_count, chapter->hours, chapter->minutes,
                 chapter->seconds );
    }
    fprintf( stderr, "  + audio tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        hb_audio_config_t *audio;
        audio = hb_list_audio_config_item( title->list_audio, i );
        if( ( audio->in.codec == HB_ACODEC_AC3 ) || ( audio->in.codec == HB_ACODEC_DCA) )
        {
            fprintf( stderr, "    + %d, %s (iso639-2: %s), %dHz, %dbps\n",
                     i + 1,
                     audio->lang.description,
                     audio->lang.iso639_2,
                     audio->in.samplerate,
                     audio->in.bitrate );
        }
        else
        {
            fprintf( stderr, "    + %d, %s (iso639-2: %s)\n",
                     i + 1,
                     audio->lang.description,
                     audio->lang.iso639_2 );
        }
    }
    fprintf( stderr, "  + subtitle tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        hb_subtitle_t *subtitle;
        subtitle = hb_list_item( title->list_subtitle, i );
        fprintf( stderr, "    + %d, %s (iso639-2: %s) (%s)(%s)\n",
                 i + 1, subtitle->lang,
                 subtitle->iso639_2,
                 (subtitle->format == TEXTSUB) ? "Text" : "Bitmap",
                 hb_subsource_name(subtitle->source));
    }

    if(title->detected_interlacing)
    {
        /* Interlacing was found in half or more of the preview frames */
        fprintf( stderr, "  + combing detected, may be interlaced or telecined\n");
    }

}

static void PrintTitleSetInfo( hb_title_set_t * title_set )
{
    int i;
    hb_title_t * title;

    for( i = 0; i < hb_list_count( title_set->list_title ); i++ )
    {
        title = hb_list_item( title_set->list_title, i );
        PrintTitleInfo( title, title_set->feature );
    }
}

static int test_sub_list( char ** list, int pos )
{
    int i;

    if ( list == NULL || pos == 0 )
        return 0;

    if ( list[0] == NULL && pos == 1 )
        return 1;

    for ( i = 0; list[i] != NULL; i++ )
    {
        int idx = strtol( list[i], NULL, 0 );
        if ( idx == pos )
            return 1;
    }
    return 0;
}

void write_chapter_names(hb_dict_t *job_dict, const char *marker_file)
{
    if (marker_file == NULL)
        return;

    hb_csv_file_t * file = hb_open_csv_file(marker_file);
    hb_csv_cell_t * cell;
    int row = 0;

    if (file == NULL)
    {
        fprintf(stderr, "Cannot open chapter marker file, using defaults\n");
        return;
    }
    fprintf(stderr, "Reading chapter markers from file %s\n", marker_file);

    hb_value_array_t *chapter_array;
    chapter_array = hb_dict_get(hb_dict_get(job_dict, "Destination"),
                                "ChapterList");

    if (chapter_array == NULL)
        return;

    /* Parse the cells */
    while (NULL != (cell = hb_read_next_cell(file)))
    {
        /* We have a chapter number */
        if (cell->cell_col == 0)
        {
            row = cell->cell_row;
        }

        /* We have a chapter name */
        if (cell->cell_col == 1 && row == cell->cell_row)
        {
            /* If we have a valid chapter, add chapter entry */
            hb_dict_t *chapter_dict = hb_value_array_get(chapter_array, row);
            if (chapter_dict != NULL)
            {
                hb_dict_set(chapter_dict, "Name",
                            hb_value_string(cell->cell_text));
            }
        }
        hb_dispose_cell( cell );
    }
    hb_close_csv_file( file );
}

static void lang_list_remove(hb_value_array_t *list, const char *lang)
{
    int count = hb_value_array_len(list);
    int ii;
    for (ii = count - 1; ii >= 0; ii--)
    {
        const char *tmp = hb_value_get_string(hb_value_array_get(list, ii));
        if (!strncmp(lang, tmp, 4))
            hb_value_array_remove(list, ii);
    }
}

static int HandleEvents(hb_handle_t * h, hb_dict_t *preset_dict)
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
            if (p.preview_cur)
            {
                fprintf(stderr, "\rScanning title %d of %d, preview %d, %.2f %%",
                        p.title_cur, p.title_count, p.preview_cur, 100 * p.progress);
            }
            else
            {
                fprintf(stderr, "\rScanning title %d of %d, %.2f %%",
                        p.title_cur, p.title_count, 100 * p.progress);
            }
            fflush(stderr);
            break;
#undef p

        case HB_STATE_SCANDONE:
        {
            hb_title_set_t * title_set;
            hb_title_t * title;

            title_set = hb_get_title_set( h );
            if( !title_set || !hb_list_count( title_set->list_title ) )
            {
                /* No valid title, stop right there */
                fprintf( stderr, "No title found.\n" );
                done_error = HB_ERROR_WRONG_INPUT;
                die = 1;
                break;
            }
            if (main_feature)
            {
                int i;
                int main_feature_idx=0;
                int main_feature_pos=-1;
                int main_feature_time=0;
                int title_time;

                fprintf( stderr, "Searching for main feature title...\n" );

                for( i = 0; i < hb_list_count( title_set->list_title ); i++ )
                {
                    title = hb_list_item( title_set->list_title, i );
                    title_time = (title->hours*60*60 ) + (title->minutes *60) + (title->seconds);
                    fprintf( stderr, " + Title (%d) index %d has length %dsec\n",
                             i, title->index, title_time );
                    if( main_feature_time < title_time )
                    {
                        main_feature_time = title_time;
                        main_feature_pos = i;
                        main_feature_idx = title->index;
                    }
                    if( title_set->feature == title->index )
                    {
                        main_feature_pos = i;
                        main_feature_idx = title->index;
                        break;
                    }
                }
                if( main_feature_pos == -1 )
                {
                    fprintf( stderr, "No main feature title found.\n" );
                    done_error = HB_ERROR_WRONG_INPUT;
                    die = 1;
                    break;
                }
                titleindex = main_feature_idx;
                fprintf(stderr, "Found main feature title %d\n",
                        main_feature_idx);

                title = hb_list_item(title_set->list_title, main_feature_pos);
            } else {
                title = hb_list_item(title_set->list_title, 0);
            }

            if (!titleindex || titlescan)
            {
                /* Scan-only mode, print infos and exit */
                PrintTitleSetInfo( title_set );
                die = 1;
                break;
            }

            fprintf( stderr, "+ Using preset: %s\n",
                hb_value_get_string(hb_dict_get(preset_dict, "PresetName")));

            PrintTitleInfo(title, title_set->feature);

            // All overrides to the preset are complete
            // Initialize the job from preset + overrides
            // and apply job specific command line overrides
            hb_dict_t *job_dict = PrepareJob(h, title, preset_dict);
            if (job_dict == NULL)
            {
                die = 1;
                return -1;
            }

            char * json_job;
            json_job = hb_value_get_json(job_dict);
            hb_value_free(&job_dict);
            if (json_job == NULL)
            {
                fprintf(stderr, "Error in setting up job! Aborting.\n");
                die = 1;
                return -1;
            }


            hb_add_json(h, json_job);
            free(json_job);
            hb_start( h );
            break;
        }

#define p s.param.working
        case HB_STATE_SEARCHING:
            fprintf( stdout, "\rEncoding: task %d of %d, Searching for start time, %.2f %%",
                     p.pass, p.pass_count, 100.0 * p.progress );
            if( p.seconds > -1 )
            {
                fprintf( stdout, " (ETA %02dh%02dm%02ds)",
                         p.hours, p.minutes, p.seconds );
            }
            fflush(stdout);
            break;

        case HB_STATE_WORKING:
            fprintf( stdout, "\rEncoding: task %d of %d, %.2f %%",
                     p.pass, p.pass_count, 100.0 * p.progress );
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
                    fprintf( stderr, "\nEncode done!\n" );
                    break;
                case HB_ERROR_CANCELED:
                    fprintf( stderr, "\nEncode canceled.\n" );
                    break;
                default:
                    fprintf( stderr, "\nEncode failed (error %x).\n",
                             p.error );
            }
            done_error = p.error;
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
    done_error = HB_ERROR_CANCELED;
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
        exit( done_error );
    }
}

/****************************************************************************
 * ShowHelp:
 ****************************************************************************/
static void ShowHelp()
{
    int i, clock_min, clock_max, clock;
    const hb_rate_t *rate;
    const hb_dither_t *dither;
    const hb_mixdown_t *mixdown;
    const hb_encoder_t *encoder;
    const hb_container_t *container;
    FILE* const out = stdout;

    fprintf( out,
"Syntax: HandBrakeCLI [options] -i <device> -o <file>\n"
"\n"
"### General Handbrake Options---------------------------------------------\n\n"
"   -h, --help              Print help\n"
"   -u, --update            Check for updates and exit\n"
"   -v, --verbose <#>       Be verbose (optional argument: logging level)\n"
"   -Z. --preset <string>   Use a built-in preset. Capitalization matters,\n"
"                           and if the preset name has spaces, surround it\n"
"                           with double quotation marks\n"
"   -z, --preset-list       See a list of available built-in presets\n"
"   --preset-import-file    Import presets from a json preset file.\n"
"       <filespec>          'filespec' may be a list of files separated\n"
"                           by spaces, or it may use shell wildcards.\n"
"   --preset-import-gui     Import presets from GUI config preset file.\n"
"   --preset-export         Create a new preset from command line options and\n"
"       <name>              write a json representation of the preset to the\n"
"                           console or a file if '--preset-export-file' is\n"
"                           specified. The required 'name' argument will be\n"
"                           the new preset's name.\n"
"   --preset-export-file    Write new preset generated by '--preset-export'\n"
"       <filename>          to file 'filename'.\n"
"   --preset-export-description\n"
"       <description>       Add a description to the new preset created with\n"
"                           '--preset-export'\n"
"       --no-dvdnav         Do not use dvdnav for reading DVDs\n"
"       --no-opencl         Disable use of OpenCL\n"
"\n"
"### Source Options--------------------------------------------------------\n\n"
"   -i, --input <string>    Set input device\n"
"   -t, --title <number>    Select a title to encode (0 to scan all titles\n"
"                           only, default: 1)\n"
"       --min-duration      Set the minimum title duration (in seconds).\n"
"                           Shorter titles will be ignored (default: 10).\n"
"       --scan              Scan selected title only.\n"
"       --main-feature      Detect and select the main feature title.\n"
"   -c, --chapters <string> Select chapters (e.g. \"1-3\" for chapters\n"
"                           1 to 3, or \"3\" for chapter 3 only,\n"
"                           default: all chapters)\n"
"       --angle <number>    Select the video angle (DVD or Blu-ray only)\n"
"       --previews <#:B>    Select how many preview images are generated,\n"
"                           and whether to store to disk (0 or 1).\n"
"                           (default: 10:0)\n"
"   --start-at-preview <#>  Start encoding at a given preview.\n"
"   --start-at    <unit:#>  Start encoding at a given frame, duration\n"
"                           (in seconds), or pts (on a 90kHz clock)\n"
"   --stop-at     <unit:#>  Stop encoding at a given frame, duration\n"
"                           (in seconds), or pts (on a 90kHz clock)"
"\n"
"### Destination Options---------------------------------------------------\n\n"
"   -o, --output <string>   Set output file name\n"
"   -f, --format <string>   Set output container format (");
    container = NULL;
    while ((container = hb_container_get_next(container)) != NULL)
    {
        fprintf(out, "%s", container->short_name);
        if (hb_container_get_next(container) != NULL)
        {
            fprintf(out, "/");
        }
        else
        {
            fprintf(out, ")\n");
        }
    }
    fprintf(out,
"                           (default: autodetected from file name)\n"
"   -m, --markers           Add chapter markers\n"
"       --no-markers        Disable preset chapter markers\n"
"   -O, --optimize          Optimize mp4 files for HTTP streaming\n"
"                           (\"fast start\")\n"
"       --no-optimize       Disable preset 'optimize'\n"
"   -I, --ipod-atom         Mark mp4 files so 5.5G iPods will accept them\n"
"       --no-ipod-atom      Disable 5.5G iPod tag\n"
"   -P, --use-opencl        Use OpenCL where applicable\n"
"   -U, --use-hwd           Use DXVA2 hardware decoding\n"
"       --no-hwd            Disable DXVA2 hardware decoding\n"
"\n"


"### Video Options------------------------------------------------------------\n\n"
"   -e, --encoder <string>  Set video library encoder\n"
"                           Options: " );
    encoder = NULL;
    while ((encoder = hb_video_encoder_get_next(encoder)) != NULL)
    {
        fprintf(out, "%s", encoder->short_name);
        if (hb_video_encoder_get_next(encoder) != NULL)
        {
            fprintf(out, "/");
        }
        else
        {
            fprintf(out, "\n");
        }
    }
    fprintf(out,
"       --encoder-preset    Adjust video encoding settings for a particular\n"
"         <string>          speed/efficiency tradeoff (encoder-specific)\n"
"   --encoder-preset-list   List supported --encoder-preset values for the\n"
"         <string>          specified video encoder\n"
"       --encoder-tune      Adjust video encoding settings for a particular\n"
"         <string>          type of souce or situation (encoder-specific)\n"
"   --encoder-tune-list     List supported --encoder-tune values for the\n"
"         <string>          specified video encoder\n"
"   -x, --encopts <string>  Specify advanced encoding options in the same\n"
"                           style as mencoder (all encoders except theora):\n"
"                           option1=value1:option2=value2\n"
"       --encoder-profile   Ensures compliance with the requested codec\n"
"         <string>          profile (encoder-specific)\n"
"   --encoder-profile-list  List supported --encoder-profile values for the\n"
"         <string>          specified video encoder\n"
"       --encoder-level     Ensures compliance with the requested codec\n"
"         <string>          level (encoder-specific)\n"
"   --encoder-level-list    List supported --encoder-level values for the\n"
"         <string>          specified video encoder\n"
"   -q, --quality <number>  Set video quality\n"
"   -b, --vb <kb/s>         Set video bitrate (default: 1000)\n"
"   -2, --two-pass          Use two-pass mode\n"
"   -T, --turbo             When using 2-pass use \"turbo\" options on the\n"
"                           1st pass to improve speed\n"
"                           (works with x264 and x265)\n"
"   -r, --rate              Set video framerate\n"
"                           (" );
    rate = NULL;
    while ((rate = hb_video_framerate_get_next(rate)) != NULL)
    {
        fprintf(out, "%s", rate->name);
        if (hb_video_framerate_get_next(rate) != NULL)
        {
            fprintf(out, "/");
        }
    }
    fprintf( out, "\n"
"                           or a number between " );
    hb_video_framerate_get_limits(&clock_min, &clock_max, &clock);
    fprintf(out, "%i and %i", (clock / clock_max), (clock / clock_min));
    fprintf( out, ").\n"
"                           Be aware that not specifying a framerate lets\n"
"                           HandBrake preserve a source's time stamps,\n"
"                           potentially creating variable framerate video\n"
"   --vfr, --cfr, --pfr     Select variable, constant or peak-limited\n"
"                           frame rate control. VFR preserves the source\n"
"                           timing. CFR makes the output constant rate at\n"
"                           the rate given by the -r flag (or the source's\n"
"                           average rate if no -r is given). PFR doesn't\n"
"                           allow the rate to go over the rate specified\n"
"                           with the -r flag but won't change the source\n"
"                           timing if it's below that rate.\n"
"                           If none of these flags are given, the default\n"
"                           is --pfr when -r is given and --vfr otherwise\n"
"\n"
"### Audio Options---------------------------------------------------------\n\n"
"       --audio-lang-list   Specifiy a comma separated list of audio\n"
"         <string>          languages you would like to select from the\n"
"                           source title. By default, the first audio\n"
"                           matching each language will be added to your\n"
"                           output. Provide the language's iso639-2 code\n"
"                           (fre, eng, spa, dut, et cetera)\n"
"                           Use code 'und' (Unknown) to match all languages.\n"
"       --all-audio         Select all audio tracks matching languages in\n"
"                           the specified language list (--audio-lang-list).\n"
"                           Any language if list is not specified.\n"
"       --first-audio       Select first audio track matching languages in\n"
"                           the specified language list (--audio-lang-list).\n"
"                           Any language if list is not specified.\n"
"   -a, --audio <string>    Select audio track(s), separated by commas\n"
"                           (\"none\" for no audio, \"1,2,3\" for multiple\n"
"                            tracks, default: first one).\n"
"                           Multiple output tracks can be used for one input.\n"
"   -E, --aencoder <string> Audio encoder(s):\n" );
    encoder = NULL;
    while ((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        fprintf(out, "                               %s\n",
                encoder->short_name);
    }
    fprintf(out,
"                           copy:* will passthrough the corresponding\n"
"                           audio unmodified to the muxer if it is a\n"
"                           supported passthrough audio type.\n"
"                           Separate tracks by commas.\n"
"                           Defaults:\n");
    container = NULL;
    while ((container = hb_container_get_next(container)) != NULL)
    {
        int audio_encoder = hb_audio_encoder_get_default(container->format);
        fprintf(out, "                               %-8s %s\n",
                container->short_name,
                hb_audio_encoder_get_short_name(audio_encoder));
    }
    fprintf(out,
"       --audio-copy-mask   Set audio codecs that are permitted when the\n"
"               <string>    \"copy\" audio encoder option is specified\n"
"                           (" );
    i       = 0;
    encoder = NULL;
    while ((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        if ((encoder->codec &  HB_ACODEC_PASS_FLAG) &&
            (encoder->codec != HB_ACODEC_AUTO_PASS))
        {
            if (i)
            {
                fprintf(out, "/");
            }
            i = 1;
            // skip "copy:"
            fprintf(out, "%s", encoder->short_name + 5);
        }
    }
    fprintf(out, ")\n"
"                           Separated by commas for multiple allowed options.\n"
"       --audio-fallback    Set audio codec to use when it is not possible\n"
"               <string>    to copy an audio track without re-encoding.\n"
"   -B, --ab <kb/s>         Set audio bitrate(s) (default: depends on the\n"
"                           selected codec, mixdown and samplerate)\n"
"                           Separate tracks by commas.\n"
"   -Q, --aq <quality>      Set audio quality metric.\n"
"                           Separate tracks by commas.\n"
"   -C, --ac <compression>  Set audio compression metric.\n"
"                           selected codec)\n"
"                           Separate tracks by commas.\n"
"   -6, --mixdown <string>  Format(s) for audio downmixing/upmixing:\n");
    // skip HB_AMIXDOWN_NONE
    mixdown = hb_mixdown_get_next(NULL);
    while((mixdown = hb_mixdown_get_next(mixdown)) != NULL)
    {
        fprintf(out, "                               %s\n",
                mixdown->short_name);
    }
    fprintf(out,
"                           Separate tracks by commas.\n"
"                           Defaults:\n");
    encoder = NULL;
    while((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        if (!(encoder->codec & HB_ACODEC_PASS_FLAG))
        {
            // layout: UINT64_MAX (all channels) should work with any mixdown
            int mixdown = hb_mixdown_get_default(encoder->codec, UINT64_MAX);
            // assumes that the encoder short name is <= 16 characters long
            fprintf(out, "                               %-16s up to %s\n",
                    encoder->short_name, hb_mixdown_get_short_name(mixdown));
        }
    }
    fprintf(out,
"       --normalize-mix     Normalize audio mix levels to prevent clipping.\n"
"              <string>     Separate tracks by commas.\n"
"                           0 = Disable Normalization (default)\n"
"                           1 = Enable Normalization\n"
"   -R, --arate             Set audio samplerate(s)\n"
"                           (" );
    rate = NULL;
    while ((rate = hb_audio_samplerate_get_next(rate)) != NULL)
    {
        fprintf(out, "%s", rate->name);
        if (hb_audio_samplerate_get_next(rate) != NULL)
        {
            fprintf(out, "/");
        }
    }
    fprintf( out, " kHz)\n"
"                           Separate tracks by commas.\n"
"   -D, --drc <float>       Apply extra dynamic range compression to the\n"
"                           audio, making soft sounds louder. Range is 1.0\n"
"                           to 4.0 (too loud), with 1.5 - 2.5 being a useful\n"
"                           range.\n"
"                           Separate tracks by commas.\n"
"       --gain <float>      Amplify or attenuate audio before encoding.  Does\n"
"                           NOT work with audio passthru (copy). Values are\n"
"                           in dB.  Negative values attenuate, positive\n"
"                           values amplify. A 1 dB difference is barely\n"
"                           audible.\n"
"       --adither <string>  Apply dithering to the audio before encoding.\n"
"                           Separate tracks by commas.\n"
"                           Only supported by some encoders\n"
"                           (");
    i       = 0;
    encoder = NULL;
    while ((encoder = hb_audio_encoder_get_next(encoder)) != NULL)
    {
        if (hb_audio_dither_is_supported(encoder->codec))
        {
            if (i)
            {
                fprintf(out, "/");
            }
            i = 1;
            fprintf(out, "%s", encoder->short_name);
        }
    }
    fprintf(out, ").\n");
    fprintf(out,
    "                            Options:\n");
    dither = NULL;
    while ((dither = hb_audio_dither_get_next(dither)) != NULL)
    {
        if (dither->method == hb_audio_dither_get_default())
        {
            fprintf(out, "                               %s (default)\n",
                    dither->short_name);
        }
        else
        {
            fprintf(out, "                               %s\n",
                    dither->short_name);
        }
    }
    fprintf(out,
"   -A, --aname <string>    Audio track name(s),\n"
"                           Separate tracks by commas.\n"
"\n"
"### Picture Settings------------------------------------------------------\n\n"
"   -w, --width  <number>   Set picture width\n"
"   -l, --height <number>   Set picture height\n"
"       --crop  <T:B:L:R>   Set cropping values (default: autocrop)\n"
"       --loose-crop        Always crop to a multiple of the modulus\n"
"       --no-loose-crop     Disable preset 'loose-crop'\n"
"   -Y, --maxHeight   <#>   Set maximum height\n"
"   -X, --maxWidth    <#>   Set maximum width\n"
"   --non-anamorphic        Set pixel aspect ratio to 1:1\n"
"   --strict-anamorphic     Store pixel aspect ratio in video stream\n"
"   --loose-anamorphic      Store pixel aspect ratio with specified width\n"
"   --custom-anamorphic     Store pixel aspect ratio in video stream and\n"
"                           directly control all parameters.\n"
"   --display-width         Set the width to scale the actual pixels to\n"
"     <number>              at playback, for custom anamorphic.\n"
"   --keep-display-aspect   Preserve the source's display aspect ratio\n"
"                           when using custom anamorphic\n"
"  --no-keep-display-aspect Disable preset 'keep-display-aspect'\n"
"   --pixel-aspect          Set a custom pixel aspect for custom anamorphic\n"
"     <PARX:PARY>           (--display-width and --pixel-aspect are mutually\n"
"                           exclusive.\n"
"   --itu-par               Use wider, ITU pixel aspect values for loose and\n"
"                           custom anamorphic, useful with underscanned\n"
"                           sources\n"
"   --no-itu-par            Disable preset 'itu-par'\n"
"   --modulus               Set the number you want the scaled pixel\n"
"                           dimensions\n"
"     <number>              to divide cleanly by. Does not affect strict\n"
"                           anamorphic mode, which is always mod 2\n"
"                           (default: 16)\n"
"   -M, --color-matrix      Set the color space signaled by the output\n"
"                           Values: 709, pal, ntsc, 601 (same as ntsc)\n"
"                           (default: detected from source)\n"
"\n"
"### Filters---------------------------------------------------------------\n\n"
"   -d, --deinterlace       Unconditionally deinterlaces all frames\n"
"         <fast/slow/slower/bob");
#ifdef USE_QSV
if (hb_qsv_available())
{
    fprintf(out, "/qsv");
}
#endif
     fprintf( out, "> or omitted (default settings)\n"
     "           or\n"
"         <YM:FD>           (default 0:-1)\n"
"       --no-deinterlace    Disable preset deinterlace filter\n"
"   -5, --decomb            Selectively deinterlaces when it detects combing\n"
"         <fast/bob> or omitted (default settings)\n"
"          or\n"
"         <MO:ME:MT:ST:BT:BX:BY:MG:VA:LA:DI:ER:NO:MD:PP:FD>\n"
"         (default: 7:2:6:9:80:16:16:10:20:20:4:2:50:24:1:-1)\n"
"       --no-decomb         Disable preset decomb filter\n"
"   -9, --detelecine        Detelecine (ivtc) video with pullup filter\n"
"                           Note: this filter drops duplicate frames to\n"
"                           restore the pre-telecine framerate, unless you\n"
"                           specify a constant framerate\n"
"                           (--rate 29.97 --cfr)\n"
"         <L:R:T:B:SB:MP:FD> (default 1:1:4:4:0:0:-1)\n"
"       --no-detelecine     Disable preset detelecine filter\n"
"   -8, --hqdn3d            Denoise video with hqdn3d filter\n"
"         <ultralight/light/medium/strong> or omitted (default settings)\n"
"          or\n"
"         <SL:SCb:SCr:TL:TCb:TCr>\n"
"         (default: 4:3:3:6:4.5:4.5)\n"
"       --no-hqdn3d         Disable preset hqdn3d filter\n"
"       --denoise           Legacy alias for '--hqdn3d'\n"
"   --nlmeans               Denoise video with nlmeans filter\n"
"         <ultralight/light/medium/strong> or omitted\n"
"          or\n"
"         <SY:OTY:PSY:RY:FY:PY:Sb:OTb:PSb:Rb:Fb:Pb:Sr:OTr:PSr:Rr:Fr:Pr>\n"
"         (default 8:1:7:3:2:0)\n"
"   --no-nlmeans            Disable preset nlmeans filter\n"
"   --nlmeans-tune          Tune nlmeans filter to content type\n"
"                           Note: only works in conjunction with presets\n"
"                           ultralight/light/medium/strong.\n"
"         <none/film/grain/highmotion/animation> or omitted (default none)\n"
"   -7, --deblock           Deblock video with pp7 filter\n"
"         <QP:M>            (default 5:2)\n"
"       --no-deblock        Disable preset deblock filter\n"
"        --rotate           Rotate image or flip its axes.\n"
"         <angle>:<mirror>  Angle rotates clockwise, can be one of:\n"
"                              0, 90, 180, 270\n"
"                           Mirror flips the image on the x axis.\n"
"                           Default: 180:0 (rotate 180 degrees)\n"
"   -g, --grayscale         Grayscale encoding\n"
"       --no-grayscale      Disable preset 'grayscale'\n"
"\n"
"### Subtitle Options------------------------------------------------------\n\n"
"      --subtitle-lang-list Specifiy a comma separated list of subtitle\n"
"          <string>         languages you would like to select from the\n"
"                           source title. By default, the first subtitle\n"
"                           matching each language will be added to your\n"
"                           output. Provide the language's iso639-2 code\n"
"                           (fre, eng, spa, dut, et cetera)\n"
"      --all-subtitles      Select all subtitle tracks matching languages in\n"
"                           the specified language list\n"
"                           (--subtitle-lang-list).\n"
"                           Any language if list is not specified.\n"
"      --first-subtitle     Select first subtitle track matching languages in\n"
"                           the specified language list\n"
"                           (--subtitle-lang-list).\n"
"                           Any language if list is not specified.\n"
"  -s, --subtitle <string>  Select subtitle track(s), separated by commas\n"
"                           More than one output track can be used for one\n"
"                           input. \"none\" for no subtitles.\n"
"                           Example: \"1,2,3\" for multiple tracks.\n"
"                           A special track name \"scan\" adds an extra 1st\n"
"                           pass. This extra pass scans subtitles matching\n"
"                           the language of the first audio or the language \n"
"                           selected by --native-language.\n"
"                           The one that's only used 10 percent of the time\n"
"                           or less is selected. This should locate subtitles\n"
"                           for short foreign language segments. Best used in\n"
"                           conjunction with --subtitle-forced.\n"
"  -F, --subtitle-forced    Only display subtitles from the selected stream\n"
"        <string>           if the subtitle has the forced flag set. The\n"
"                           values in \"string\" are indexes into the\n"
"                           subtitle list specified with '--subtitle'.\n"
"                           Separate tracks by commas.\n"
"                           Example: \"1,2,3\" for multiple tracks.\n"
"                           If \"string\" is omitted, the first track is\n"
"                           forced.\n"
"      --subtitle-burned    \"Burn\" the selected subtitle into the video\n"
"        <subtitle>         track. If \"subtitle\" is omitted, the first\n"
"                           track is burned. \"subtitle\" is an index into\n"
"                           the subtitle list specified with '--subtitle'\n"
"                           or \"native\" to burn the subtitle track that may\n"
"                           be added by the 'native-language' option.\n"
"      --subtitle-default   Flag the selected subtitle as the default\n"
"        <number>           subtitle to be displayed upon playback.  Setting\n"
"                           no default means no subtitle will be displayed\n"
"                           automatically. \"number\" is an index into the\n"
"                           subtitle list specified with '--subtitle'.\n"
"  -N, --native-language    Specifiy your language preference. When the first\n"
"      <string>             audio track does not match your native language\n"
"                           then select the first subtitle that does. When\n"
"                           used in conjunction with --native-dub the audio\n"
"                           track is changed in preference to subtitles.\n"
"                           Provide the language's iso639-2 code:\n"
"                               (fre, eng, spa, dut, et cetera)\n"
"      --native-dub         Used in conjunction with --native-language\n"
"                           requests that if no audio tracks are selected the\n"
"                           default selected audio track will be the first\n"
"                           one that matches the --native-language. If there\n"
"                           are no matching audio tracks then the first\n"
"                           matching subtitle track is used instead.\n"
"     --srt-file <string>   SubRip SRT filename(s), separated by commas.\n"
"     --srt-codeset         Character codeset(s) that the SRT file(s) are\n"
"       <string>            encoded in, separated by commas.\n"
"                           Use 'iconv -l' for a list of valid\n"
"                           codesets. If not specified, 'latin1' is assumed\n"
"     --srt-offset          Offset (in milliseconds) to apply to the SRT\n"
"       <string>            file(s), separated by commas. If not specified,\n"
"                           zero is assumed. Offsets may be negative.\n"
"     --srt-lang <string>   SRT track language as an iso639-2 code:\n"
"                               (fre, eng, spa, dut, et cetera)\n"
"                           Separated by commas. If not specified, then 'und'\n"
"                           is used.\n"
"     --srt-default         Flag the selected srt as the default subtitle\n"
"       <number>            to be displayed upon playback. Setting no default\n"
"                           means no subtitle will be automatically displayed\n"
"                           If \"number\" is omitted, the first SRT is the\n"
"                           default. \"number\" is an 1 based index into the\n"
"                           'srt-file' list\n"
"     --srt-burn            \"Burn\" the selected SRT subtitle into the\n"
"       <number>            video track. If \"number\" is omitted, the first\n"
"                           SRT is burned. \"number\" is an 1 based index\n"
"                           into the 'srt-file' list\n"
"\n"
    );

#ifdef USE_QSV
if (hb_qsv_available())
{
    fprintf( out,
"### Intel Quick Sync Video------------------------------------------------\n\n"
"   --disable-qsv-decoding  Force software decoding of the video track.\n"
"   --enable-qsv-decoding   Allow QSV hardware decoding of the video track.\n"
"   --qsv-async-depth       Specifies how many asynchronous operations\n"
"                           should be performed before the result is\n"
"                           explicitly synchronized.\n"
"                           Default: 4. If zero, the value is not specified.\n"
"\n"
    );
}
#endif
}

/****************************************************************************
 * ShowPresets:
 ****************************************************************************/
static const char *
reverse_search_char(const char *front, const char *back, char delim)
{
    while (back != front && *back != delim)
        back--;
    return back;
}

#if defined( __MINGW32__ )
static char * my_strndup(const char *src, int len)
{
    int src_len = strlen(src);
    int alloc = src_len < len ? src_len + 1 : len + 1;
    char *result = malloc(alloc);
    strncpy(result, src, alloc - 1);
    result[alloc - 1] = 0;
    return result;
}
#else
#define my_strndup strndup
#endif

static char** str_width_split( const char *str, int width )
{
    const char *  pos;
    const char *  end;
    char ** ret;
    int     count, ii;
    int     len;
    char    delem = ' ';

    if ( str == NULL || str[0] == 0 )
    {
        ret = malloc( sizeof(char*) );
        if ( ret == NULL ) return ret;
        *ret = NULL;
        return ret;
    }

    len = strlen(str);

    // Find number of elements in the string
    count = 1;
    pos = str;
    end = pos + width;
    while (end < str + len)
    {
        end = reverse_search_char(pos, end, delem);
        if (end == pos)
        {
            // Shouldn't happen for reasonable input
            break;
        }
        count++;
        pos = end + 1;
        end = pos + width;
    }
    count++;
    ret = calloc( ( count + 1 ), sizeof(char*) );
    if ( ret == NULL ) return ret;
    
    pos = str;
    end = pos + width;
    for (ii = 0; ii < count - 1 && end < str + len; ii++)
    {
        end = reverse_search_char(pos, end, delem);
        if (end == pos)
        {
            break;
        }
        ret[ii] = my_strndup(pos, end - pos);
        pos = end + 1;
        end = pos + width;
    }
    if (*pos != 0 && ii < count - 1)
    {
        ret[ii] = my_strndup(pos, width);
    }

    return ret;
}

static void Indent(FILE *f, char *whitespace, int indent)
{
    int ii;
    for (ii = 0; ii < indent; ii++)
    {
        fprintf(f, "%s", whitespace);
    }
}

static void ShowPresets(hb_value_array_t *presets, int indent, int descriptions)
{
    if (presets == NULL)
        presets = hb_presets_get();

    int count = hb_value_array_len(presets);
    int ii;
    for (ii = 0; ii < count; ii++)
    {
        const char *name;
        hb_dict_t *preset_dict = hb_value_array_get(presets, ii);
        name = hb_value_get_string(hb_dict_get(preset_dict, "PresetName"));
        Indent(stderr, "    ", indent);
        if (hb_value_get_bool(hb_dict_get(preset_dict, "Folder")))
        {
            indent++;
            fprintf(stderr, "%s/\n", name);
            hb_value_array_t *children;
            children = hb_dict_get(preset_dict, "ChildrenArray");
            if (children == NULL)
                continue;
            ShowPresets(children, indent, descriptions);
            indent--;
        }
        else
        {
            fprintf(stderr, "%s\n", name);
            if (descriptions)
            {
                const char *desc;
                desc = hb_value_get_string(hb_dict_get(preset_dict,
                                                       "PresetDescription"));
                if (desc != NULL && desc[0] != 0)
                {
                    int ii;
                    char **split = str_width_split(desc, 60);
                    for (ii = 0; split[ii] != NULL; ii++)
                    {
                        Indent(stderr, "    ", indent+1);
                        fprintf(stderr, "%s\n", split[ii]);
                    }
                    str_vfree(split);
                }
            }
        }
    }
}

static char* strchr_quote(char *pos, char c, char q)
{
    if (pos == NULL)
        return NULL;

    while (*pos != 0 && *pos != c)
    {
        if (*pos == q)
        {
            pos = strchr_quote(pos+1, q, 0);
            if (pos == NULL)
                return NULL;
            pos++;
        }
        else if (*pos == '\\' && *(pos+1) != 0)
            pos += 2;
        else
            pos++;
    }
    if (*pos != c)
        return NULL;
    return pos;
}

static char *strndup_quote(char *str, char q, int len)
{
    if (str == NULL)
        return NULL;

    char * res;
    int str_len = strlen( str );
    int src = 0, dst = 0;
    res = malloc( len > str_len ? str_len + 1 : len + 1 );
    if ( res == NULL ) return res;

    while (str[src] != 0 && src < len)
    {
        if (str[src] == q)
            src++;
        else if (str[src] == '\\' && str[src+1] != 0)
        {
            res[dst++] = str[src+1];
            src += 2;
        }
        else
            res[dst++] = str[src++];
    }
    res[dst] = '\0';
    return res;
}

static int str_vlen(char **strv)
{
    int i;
    if (strv == NULL)
        return 0;
    for (i = 0; strv[i]; i++);
    return i;
}

static char** str_split( char *str, char delem )
{
    char *  pos;
    char *  end;
    char ** ret;
    int     count, i;
    char quote = '"';

    if (delem == '"')
    {
        quote = '\'';
    }
    if ( str == NULL || str[0] == 0 )
    {
        ret = malloc( sizeof(char*) );
        if ( ret == NULL ) return ret;
        *ret = NULL;
        return ret;
    }

    // Find number of elements in the string
    count = 1;
    pos = str;
    while ( ( pos = strchr_quote( pos, delem, quote ) ) != NULL )
    {
        count++;
        pos++;
    }

    ret = calloc( ( count + 1 ), sizeof(char*) );
    if ( ret == NULL ) return ret;

    pos = str;
    for ( i = 0; i < count - 1; i++ )
    {
        end = strchr_quote( pos, delem, quote );
        ret[i] = strndup_quote(pos, quote, end - pos);
        pos = end + 1;
    }
    ret[i] = strndup_quote(pos, quote, strlen(pos));

    return ret;
}

static void str_vfree( char **strv )
{
    int i;

    if (strv == NULL)
        return;

    for ( i = 0; strv[i]; i++ )
    {
        free( strv[i] );
    }
    free( strv );
}

static double parse_hhmmss_strtok()
{
    /* Assumes strtok has already been called on a string.  Intends to parse
     * hh:mm:ss.ss or mm:ss.ss or ss.ss or ss into double seconds.  Actually
     * parses a list of doubles separated by colons, multiplying the current
     * result by 60 then adding in the next value.  Malformed input does not
     * result in a explicit error condition but instead returns an
     * intermediate result. */
    double duration = 0;
    char* str;
    while ((str = strtok(NULL, ":")) != NULL)
        duration = 60*duration + strtod(str, NULL);
    return duration;
}

/****************************************************************************
 * ParseOptions:
 ****************************************************************************/
static int ParseOptions( int argc, char ** argv )
{

    #define PREVIEWS             257
    #define START_AT_PREVIEW     258
    #define START_AT             259
    #define STOP_AT              260
    #define ANGLE                261
    #define DVDNAV               262
    #define DISPLAY_WIDTH        263
    #define PIXEL_ASPECT         264
    #define MODULUS              265
    #define KEEP_DISPLAY_ASPECT  266
    #define SUB_BURNED           267
    #define SUB_DEFAULT          268
    #define NATIVE_DUB           269
    #define SRT_FILE             270
    #define SRT_CODESET          271
    #define SRT_OFFSET           272
    #define SRT_LANG             273
    #define SRT_DEFAULT          274
    #define SRT_BURN             275
    #define ROTATE_FILTER        276
    #define SCAN_ONLY            277
    #define MAIN_FEATURE         278
    #define MIN_DURATION         279
    #define AUDIO_GAIN           280
    #define ALLOWED_AUDIO_COPY   281
    #define AUDIO_FALLBACK       282
    #define LOOSE_CROP           283
    #define ENCODER_PRESET       284
    #define ENCODER_PRESET_LIST  285
    #define ENCODER_TUNE         286
    #define ENCODER_TUNE_LIST    287
    #define ENCODER_PROFILE      288
    #define ENCODER_PROFILE_LIST 289
    #define ENCODER_LEVEL        290
    #define ENCODER_LEVEL_LIST   291
    #define NORMALIZE_MIX        293
    #define AUDIO_DITHER         294
    #define QSV_BASELINE         295
    #define QSV_ASYNC_DEPTH      296
    #define QSV_IMPLEMENTATION   297
    #define FILTER_NLMEANS       298
    #define FILTER_NLMEANS_TUNE  299
    #define AUDIO_LANG_LIST      300
    #define SUBTITLE_LANG_LIST   301
    #define PRESET_EXPORT        302
    #define PRESET_EXPORT_DESC   303
    #define PRESET_EXPORT_FILE   304
    #define PRESET_IMPORT        305
    #define PRESET_IMPORT_GUI    306
    #define VERSION              307
    #define DESCRIBE             308

    for( ;; )
    {
        static struct option long_options[] =
          {
            { "help",        no_argument,       NULL,    'h' },
            { "version",     no_argument,       NULL,    VERSION },
            { "describe",    no_argument,       NULL,    DESCRIBE },
            { "update",      no_argument,       NULL,    'u' },
            { "verbose",     optional_argument, NULL,    'v' },
            { "no-dvdnav",   no_argument,       NULL,    DVDNAV },
            { "no-opencl",   no_argument,       &use_opencl, 0 },

#ifdef USE_QSV
            { "qsv-baseline",         no_argument,       NULL,        QSV_BASELINE,       },
            { "qsv-async-depth",      required_argument, NULL,        QSV_ASYNC_DEPTH,    },
            { "qsv-implementation",   required_argument, NULL,        QSV_IMPLEMENTATION, },
            { "disable-qsv-decoding", no_argument,       &qsv_decode, 0,                  },
            { "enable-qsv-decoding",  no_argument,       &qsv_decode, 1,                  },
#endif

            { "format",      required_argument, NULL,    'f' },
            { "input",       required_argument, NULL,    'i' },
            { "output",      required_argument, NULL,    'o' },
            { "optimize",    no_argument,       NULL,        'O' },
            { "no-optimize", no_argument,       &mp4_optimize, 0 },
            { "ipod-atom",   no_argument,       NULL,        'I' },
            { "no-ipod-atom",no_argument,       &ipod_atom,    0 },
            { "use-opencl",  no_argument,       NULL,        'P' },
            { "use-hwd",     no_argument,       NULL,        'U' },
            { "no-hwd",      no_argument,       &use_hwd,      0 },

            { "title",       required_argument, NULL,    't' },
            { "min-duration",required_argument, NULL,    MIN_DURATION },
            { "scan",        no_argument,       NULL,    SCAN_ONLY },
            { "main-feature",no_argument,       NULL,    MAIN_FEATURE },
            { "chapters",    required_argument, NULL,    'c' },
            { "angle",       required_argument, NULL,    ANGLE },
            { "markers",     optional_argument, NULL,    'm' },
            { "no-markers",  no_argument,       &chapter_markers, 0 },
            { "audio-lang-list", required_argument, NULL, AUDIO_LANG_LIST },
            { "all-audio",   no_argument,       &audio_all, 1 },
            { "first-audio", no_argument,       &audio_all, 0 },
            { "audio",       required_argument, NULL,    'a' },
            { "mixdown",     required_argument, NULL,    '6' },
            { "normalize-mix", required_argument, NULL,  NORMALIZE_MIX },
            { "drc",         required_argument, NULL,    'D' },
            { "gain",        required_argument, NULL,    AUDIO_GAIN },
            { "adither",     required_argument, NULL,    AUDIO_DITHER },
            { "subtitle-lang-list", required_argument, NULL, SUBTITLE_LANG_LIST },
            { "all-subtitles", no_argument,     &subtitle_all, 1 },
            { "first-subtitle", no_argument,    &subtitle_all, 0 },
            { "subtitle",    required_argument, NULL,    's' },
            { "subtitle-forced", optional_argument,   NULL,    'F' },
            { "subtitle-burned", optional_argument,   NULL,    SUB_BURNED },
            { "subtitle-default", optional_argument,   NULL,    SUB_DEFAULT },
            { "srt-file",    required_argument, NULL, SRT_FILE },
            { "srt-codeset", required_argument, NULL, SRT_CODESET },
            { "srt-offset",  required_argument, NULL, SRT_OFFSET },
            { "srt-lang",    required_argument, NULL, SRT_LANG },
            { "srt-default", optional_argument, NULL, SRT_DEFAULT },
            { "srt-burn",    optional_argument, NULL, SRT_BURN },
            { "native-language", required_argument, NULL,'N' },
            { "native-dub",  no_argument,       NULL,    NATIVE_DUB },
            { "encoder",     required_argument, NULL,    'e' },
            { "aencoder",    required_argument, NULL,    'E' },
            { "two-pass",    no_argument,       NULL,    '2' },
            { "deinterlace", optional_argument, NULL,    'd' },
            { "no-deinterlace", no_argument,    &deinterlace_disable, 1 },
            { "deblock",     optional_argument, NULL,    '7' },
            { "no-deblock",  no_argument,       &deblock_disable,     1 },
            { "denoise",     optional_argument, NULL,    '8' },
            { "hqdn3d",      optional_argument, NULL,    '8' },
            { "no-hqdn3d",   no_argument,       &hqdn3d_disable,      1 },
            { "nlmeans",     optional_argument, NULL,    FILTER_NLMEANS },
            { "no-nlmeans",  no_argument,       &nlmeans_disable,     1 },
            { "nlmeans-tune",required_argument, NULL,    FILTER_NLMEANS_TUNE },
            { "detelecine",  optional_argument, NULL,    '9' },
            { "no-detelecine", no_argument,     &detelecine_disable,  1 },
            { "decomb",      optional_argument, NULL,    '5' },
            { "no-decomb",   no_argument,       &decomb_disable,      1 },
            { "grayscale",   no_argument,       NULL,        'g' },
            { "no-grayscale",no_argument,       &grayscale,    0 },
            { "rotate",      optional_argument, NULL,   ROTATE_FILTER },
            { "non-anamorphic",  no_argument, &anamorphic_mode, 0 },
            { "strict-anamorphic",  no_argument, &anamorphic_mode, 1 },
            { "loose-anamorphic", no_argument, &anamorphic_mode, 2 },
            { "custom-anamorphic", no_argument, &anamorphic_mode, 3 },
            { "display-width", required_argument, NULL, DISPLAY_WIDTH },
            { "keep-display-aspect", optional_argument, NULL, KEEP_DISPLAY_ASPECT },
            { "no-keep-display-aspect", no_argument, &keep_display_aspect, 0 },
            { "pixel-aspect", required_argument, NULL, PIXEL_ASPECT },
            { "modulus",     required_argument, NULL, MODULUS },
            { "itu-par",     no_argument,       &itu_par, 1  },
            { "no-itu-par",  no_argument,       &itu_par, 0  },
            { "width",       required_argument, NULL,    'w' },
            { "height",      required_argument, NULL,    'l' },
            { "crop",        required_argument, NULL,    'n' },
            { "loose-crop",  optional_argument, NULL, LOOSE_CROP },
            { "no-loose-crop", no_argument,     &loose_crop, 0 },

            // mapping of legacy option names for backwards compatibility
            { "qsv-preset",           required_argument, NULL, ENCODER_PRESET,       },
            { "x264-preset",          required_argument, NULL, ENCODER_PRESET,       },
            { "x265-preset",          required_argument, NULL, ENCODER_PRESET,       },
            { "x264-tune",            required_argument, NULL, ENCODER_TUNE,         },
            { "x265-tune",            required_argument, NULL, ENCODER_TUNE,         },
            { "x264-profile",         required_argument, NULL, ENCODER_PROFILE,      },
            { "h264-profile",         required_argument, NULL, ENCODER_PROFILE,      },
            { "h265-profile",         required_argument, NULL, ENCODER_PROFILE,      },
            { "h264-level",           required_argument, NULL, ENCODER_LEVEL,        },
            { "h265-level",           required_argument, NULL, ENCODER_LEVEL,        },
            // encoder preset/tune/options/profile/level
            { "encoder-preset",       required_argument, NULL, ENCODER_PRESET,       },
            { "encoder-preset-list",  required_argument, NULL, ENCODER_PRESET_LIST,  },
            { "encoder-tune",         required_argument, NULL, ENCODER_TUNE,         },
            { "encoder-tune-list",    required_argument, NULL, ENCODER_TUNE_LIST,    },
            { "encopts",              required_argument, NULL, 'x',                  },
            { "encoder-profile",      required_argument, NULL, ENCODER_PROFILE,      },
            { "encoder-profile-list", required_argument, NULL, ENCODER_PROFILE_LIST, },
            { "encoder-level",        required_argument, NULL, ENCODER_LEVEL,        },
            { "encoder-level-list",   required_argument, NULL, ENCODER_LEVEL_LIST,   },

            { "vb",          required_argument, NULL,    'b' },
            { "quality",     required_argument, NULL,    'q' },
            { "ab",          required_argument, NULL,    'B' },
            { "aq",          required_argument, NULL,    'Q' },
            { "ac",          required_argument, NULL,    'C' },
            { "rate",        required_argument, NULL,    'r' },
            { "arate",       required_argument, NULL,    'R' },
            { "turbo",       no_argument,       NULL,    'T' },
            { "maxHeight",   required_argument, NULL,    'Y' },
            { "maxWidth",    required_argument, NULL,    'X' },
            { "preset",      required_argument, NULL,    'Z' },
            { "preset-list", no_argument,       NULL,    'z' },
            { "preset-import-file", no_argument, NULL, PRESET_IMPORT },
            { "preset-import-gui",  no_argument,       NULL, PRESET_IMPORT_GUI },
            { "preset-export",      required_argument, NULL, PRESET_EXPORT },
            { "preset-export-file", required_argument, NULL, PRESET_EXPORT_FILE },
            { "preset-export-description", required_argument, NULL, PRESET_EXPORT_DESC },

            { "aname",       required_argument, NULL,    'A' },
            { "color-matrix",required_argument, NULL,    'M' },
            { "previews",    required_argument, NULL,    PREVIEWS },
            { "start-at-preview", required_argument, NULL, START_AT_PREVIEW },
            { "start-at",    required_argument, NULL,    START_AT },
            { "stop-at",    required_argument, NULL,     STOP_AT },
            { "vfr",         no_argument,       &cfr,    0 },
            { "cfr",         no_argument,       &cfr,    1 },
            { "pfr",         no_argument,       &cfr,    2 },
            { "audio-copy-mask", required_argument, NULL, ALLOWED_AUDIO_COPY },
            { "audio-fallback",  required_argument, NULL, AUDIO_FALLBACK },
            { 0, 0, 0, 0 }
          };

        int option_index = 0;
        int c;
        int cur_optind;

        cur_optind = optind;
        c = getopt_long( argc, argv,
                         "hv::uC:f:4i:Io:PUt:c:m::M:a:A:6:s:F::N:e:E:Q:C:"
                         "2dD:7895gOw:l:n:b:q:S:B:r:R:x:TY:X:Z:z",
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
            case VERSION:
                printf("HandBrake %s\n", hb_get_version(NULL));
                exit(0);
            case DESCRIBE:
                printf("%s\n", hb_get_full_description());
                exit(0);
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
            case 'Z':
                preset_name = strdup(optarg);
                break;
            case 'z':
                ShowPresets(NULL, 0, 1);
                exit ( 0 );
            case PRESET_EXPORT:
                preset_export_name = strdup(optarg);
                break;
            case PRESET_EXPORT_DESC:
                preset_export_desc = strdup(optarg);
                break;
            case PRESET_EXPORT_FILE:
                preset_export_file = strdup(optarg);
                break;
            case PRESET_IMPORT:
            {
                // Import list of preset files
                while (optind < argc && argv[optind][0] != '-')
                {
                    int result = hb_presets_add_path(argv[optind]);
                    if (result < 0)
                    {
                        fprintf(stderr, "Preset import failed, file (%s)\n",
                                argv[optind]);
                    }
                    optind++;
                }
            }   break;
            case PRESET_IMPORT_GUI:
                hb_presets_gui_init();
                break;
            case DVDNAV:
                dvdnav = 0;
                break;

            case 'f':
                format = strdup( optarg );
                break;
            case 'i':
                input = strdup( optarg );
#ifdef __APPLE_CC__
                char *devName = bsd_name_for_path( input ); // alloc
                if( devName != NULL )
                {
                    if( device_is_dvd( devName ))
                    {
                        free( input );
                        input = malloc( strlen( "/dev/" ) + strlen( devName ) + 1 );
                        if( input == NULL )
                        {
                            fprintf( stderr, "ERROR: malloc() failed while attempting to set device path.\n" );
                            free( devName );
                            return -1;
                        }
                        sprintf( input, "/dev/%s", devName );
                    }
                    free( devName );
                }
#endif
                break;
            case 'o':
                output = strdup( optarg );
                break;
            case 'O':
                mp4_optimize = 1;
                break;
            case 'I':
                ipod_atom = 1;
                break;
            case 'P':
                use_opencl = 1;
                break;
            case 'U':
                use_hwd = 1;
                break;
            case 't':
                titleindex = atoi( optarg );
                break;
            case SCAN_ONLY:
                titlescan = 1;
                break;
            case MAIN_FEATURE:
                main_feature = 1;
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
                    fprintf( stderr, "chapters: Invalid syntax (%s)\n",
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
            case AUDIO_LANG_LIST:
                audio_lang_list = str_split(optarg, ',');
                break;
            case SUBTITLE_LANG_LIST:
                subtitle_lang_list = str_split(optarg, ',');
                break;
            case 'a':
                if( optarg != NULL )
                {
                    atracks = str_split(optarg, ',');
                }
                else
                {
                    atracks = str_split("1", ',');
                }
                break;
            case '6':
                if( optarg != NULL )
                {
                    mixdowns = str_split(optarg, ',');
                }
                break;
            case 'D':
                if( optarg != NULL )
                {
                    dynamic_range_compression = str_split(optarg, ',');
                }
                break;
            case AUDIO_GAIN:
                if( optarg != NULL )
                {
                    audio_gain = str_split(optarg, ',');
                }
                break;
            case AUDIO_DITHER:
                if (optarg != NULL)
                {
                    audio_dither = str_split(optarg, ',');
                }
                break;
            case NORMALIZE_MIX:
                normalize_mix_level = str_split(optarg, ',');
                break;
            case 's':
                subtracks = str_split( optarg, ',' );
                break;
            case 'F':
                subforce = str_split( optarg, ',' );
                break;
            case SUB_BURNED:
                if (optarg != NULL)
                {
                    if (!strcasecmp(optarg, "native") ||
                        !strcasecmp(optarg, "scan"))
                        subburn_native = 1;
                    else
                    {
                        subburn = strtol(optarg, NULL, 0);
                    }
                }
                else
                {
                    subburn = 1;
                }
                if (subburn > 0)
                {
                    if (subtracks != NULL && str_vlen(subtracks) >= subburn &&
                        !strcasecmp("scan", subtracks[subburn-1]))
                    {
                        subburn_native = 1;
                    }
                }
                break;
            case SUB_DEFAULT:
                if (optarg != NULL)
                {
                    subdefault = strtol(optarg, NULL, 0);
                }
                else
                {
                    subdefault = 1;
                }
                break;
            case 'N':
            {
                const iso639_lang_t *lang = lang_lookup(optarg);
                if (lang != NULL)
                {
                    native_language = strdup(lang->iso639_2);
                }
                else
                {
                    fprintf(stderr, "Invalid native language (%s)\n", optarg);
                    return -1;
                }
            }   break;
            case NATIVE_DUB:
                native_dub = 1;
                break;
            case SRT_FILE:
                srtfile = str_split( optarg, ',' );
                break;
            case SRT_CODESET:
                srtcodeset = str_split( optarg, ',' );
                break;
            case SRT_OFFSET:
                srtoffset = str_split( optarg, ',' );
                break;
            case SRT_LANG:
                srtlang = str_split( optarg, ',' );
                break;
            case SRT_DEFAULT:
                if( optarg != NULL )
                {
                    srtdefault = atoi( optarg );
                }
                else
                {
                    srtdefault = 1 ;
                }
                break;
            case SRT_BURN:
                if( optarg != NULL )
                {
                    srtburn = atoi( optarg );
                }
                else
                {
                    srtburn = 1 ;
                }
                break;
            case '2':
                twoPass = 1;
                break;
            case 'd':
                free(deinterlace);
                if (optarg != NULL)
                {
                    deinterlace = strdup(optarg);
                }
                else
                {
                    deinterlace = strdup("default");
                }
                break;
            case '7':
                free(deblock);
                if( optarg != NULL )
                {
                    deblock = strdup(optarg);
                }
                else
                {
                    deblock = strdup("5");
                }
                break;
            case '8':
                free(hqdn3d);
                if (optarg != NULL)
                {
                    hqdn3d = strdup(optarg);
                }
                else
                {
                    hqdn3d = strdup("default");
                }
                break;
            case FILTER_NLMEANS:
                free(nlmeans);
                if (optarg != NULL)
                {
                    nlmeans = strdup(optarg);
                }
                else
                {
                    nlmeans = strdup("medium");
                }
                break;
            case FILTER_NLMEANS_TUNE:
                free(nlmeans_tune);
                nlmeans_tune = strdup(optarg);
                break;
            case '9':
                free(detelecine);
                if (optarg != NULL)
                {
                    detelecine = strdup(optarg);
                }
                else
                {
                    detelecine = strdup("default");
                }
                break;
            case '5':
                free(decomb);
                if (optarg != NULL)
                {
                    decomb = strdup(optarg);
                }
                else
                {
                    decomb = strdup("default");
                }
                break;
            case 'g':
                grayscale = 1;
                break;
            case ROTATE_FILTER:
                free(rotate);
                if (optarg != NULL)
                {
                    rotate = strdup(optarg);
                }
                else
                {
                    rotate = strdup("180:0");
                }
                break;
            case KEEP_DISPLAY_ASPECT:
                if( optarg != NULL )
                {
                    keep_display_aspect = atoi(optarg);
                }
                else
                {
                    keep_display_aspect = 1;
                }
                break;
            case DISPLAY_WIDTH:
                if( optarg != NULL )
                {
                    display_width = atoi(optarg);
                }
                break;
            case PIXEL_ASPECT:
                if( optarg != NULL )
                {
                    sscanf(optarg, "%i:%i", &par_width, &par_height);
                }
                break;
            case MODULUS:
                if( optarg != NULL )
                {
                    modulus = atoi(optarg);
                }
                break;
            case 'e':
            {
                vcodec = strdup(optarg);
                break;
            }
            case 'E':
                if( optarg != NULL )
                {
                    acodecs = str_split(optarg, ',');
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
            case LOOSE_CROP:
                if (optarg != NULL)
                    loose_crop = atoi(optarg);
                else
                    loose_crop = 1;
                break;
            case 'r':
            {
                vrate = strdup(optarg);
                if ( cfr != 2 )
                {
                    cfr = 2;
                }
                break;
            }
            case 'R':
                arates = str_split( optarg, ',' );
                break;
            case 'b':
                vbitrate = atoi( optarg );
                break;
            case 'q':
                vquality = atof( optarg );
                break;
            case 'B':
                abitrates = str_split( optarg, ',' );
                break;
            case 'Q':
                aqualities = str_split( optarg, ',' );
                break;
            case 'C':
                acompressions = str_split( optarg, ',' );
                break;
            case ENCODER_PRESET:
                encoder_preset = strdup( optarg );
                break;
            case ENCODER_TUNE:
                encoder_tune = strdup( optarg );
                break;
            case 'x':
                advanced_opts = strdup( optarg );
                break;
            case ENCODER_PROFILE:
                encoder_profile = strdup( optarg );
                break;
            case ENCODER_LEVEL:
                encoder_level = strdup( optarg );
                break;
            case ENCODER_PRESET_LIST:
                fprintf(stderr, "Available --encoder-preset values for '%s' encoder:\n",
                        hb_video_encoder_get_short_name(hb_video_encoder_get_from_name(optarg)));
                print_string_list(stderr,
                                  hb_video_encoder_get_presets(hb_video_encoder_get_from_name(optarg)),
                                  "    ");
                exit(0);
            case ENCODER_TUNE_LIST:
                fprintf(stderr, "Available --encoder-tune values for '%s' encoder:\n",
                        hb_video_encoder_get_short_name(hb_video_encoder_get_from_name(optarg)));
                print_string_list(stderr,
                                  hb_video_encoder_get_tunes(hb_video_encoder_get_from_name(optarg)),
                                  "    ");
                exit(0);
            case ENCODER_PROFILE_LIST:
                fprintf(stderr, "Available --encoder-profile values for '%s' encoder:\n",
                        hb_video_encoder_get_short_name(hb_video_encoder_get_from_name(optarg)));
                print_string_list(stderr,
                                  hb_video_encoder_get_profiles(hb_video_encoder_get_from_name(optarg)),
                                  "    ");
                exit(0);
            case ENCODER_LEVEL_LIST:
                fprintf(stderr, "Available --encoder-level values for '%s' encoder:\n",
                        hb_video_encoder_get_short_name(hb_video_encoder_get_from_name(optarg)));
                print_string_list(stderr,
                                  hb_video_encoder_get_levels(hb_video_encoder_get_from_name(optarg)),
                                  "    ");
                exit(0);
            case 'T':
                fastfirstpass = 1;
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
                    anames = str_split( optarg, ',' );
                }
                break;
            case PREVIEWS:
                sscanf( optarg, "%i:%i", &preview_count, &store_previews );
                break;
            case START_AT_PREVIEW:
                start_at_preview = atoi( optarg );
                break;
            case START_AT:
            {
                char * start_at_string = NULL;
                char * start_at_token = NULL;
                start_at_string = strdup( optarg );
                start_at_token = strtok( start_at_string, ":");
                if( !strcmp( start_at_token, "frame" ) )
                {
                    start_at_token = strtok( NULL, ":");
                    start_at_frame = atoi(start_at_token);
                }
                else if( !strcmp( start_at_token, "pts" ) )
                {
                    start_at_token = strtok( NULL, ":");
                    sscanf( start_at_token, "%"SCNd64, &start_at_pts );
                }
                else if( !strcmp( start_at_token, "duration" ) )
                {
                    double duration_seconds = parse_hhmmss_strtok();
                    start_at_pts = (int64_t)(duration_seconds * 90e3);
                }
                free( start_at_string );
                break;
            }
            case STOP_AT:
            {
                char * stop_at_string = NULL;
                char * stop_at_token = NULL;
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
                    double duration_seconds = parse_hhmmss_strtok();
                    stop_at_pts = (int64_t)(duration_seconds * 90e3);
                }
                free( stop_at_string );
                break;
            }
            case ALLOWED_AUDIO_COPY:
            {
                audio_copy_list = str_split(optarg, ',');
                break;
            }
            case AUDIO_FALLBACK:
                acodec_fallback = strdup( optarg );
                break;
            case 'M':
                if( optarg != NULL )
                {
                    if( !strcmp( optarg, "601" ) ||
                        !strcmp( optarg, "ntsc" ) )
                        color_matrix_code = 1;
                    else if( !strcmp( optarg, "pal" ) )
                        color_matrix_code = 2;
                    else if( !strcmp( optarg, "709" ) )
                        color_matrix_code = 3;
                } break;
            case MIN_DURATION:
                min_title_duration = strtol( optarg, NULL, 0 );
                break;
#ifdef USE_QSV
            case QSV_BASELINE:
                hb_qsv_force_workarounds();
                break;
            case QSV_ASYNC_DEPTH:
                qsv_async_depth = atoi(optarg);
                break;
            case QSV_IMPLEMENTATION:
                hb_qsv_impl_set_preferred(optarg);
                break;
#endif
            default:
                fprintf( stderr, "unknown option (%s)\n", argv[cur_optind] );
                return -1;
        }

    }

    if (deblock != NULL)
    {
        if (deblock_disable)
        {
            fprintf(stderr,
                    "Incompatible options --deblock and --no-deblock\n");
            return -1;
        }
        if (hb_validate_filter_settings(HB_FILTER_DEBLOCK, deblock))
        {
            fprintf(stderr, "Invalid deblock option %s\n", deblock);
            return -1;
        }
    }

    if (detelecine != NULL)
    {
        if (detelecine_disable)
        {
            fprintf(stderr,
                    "Incompatible options --detelecine and --no-detelecine\n");
            return -1;
        }
        if (!hb_validate_filter_preset(HB_FILTER_DETELECINE,
                                       detelecine, NULL))
        {
            // Nothing to do, but must validate preset before
            // attempting to validate custom settings to prevent potential
            // false positive
        }
        else if (!hb_validate_filter_settings(HB_FILTER_DETELECINE, detelecine))
        {
            detelecine_custom = 1;
        }
        else
        {
            fprintf(stderr, "Invalid detelecine option %s\n", detelecine);
            return -1;
        }
    }

    if (deinterlace != NULL)
    {
        if (deinterlace_disable)
        {
            fprintf(stderr,
                    "Incompatible options --deinterlace and --no-deinterlace\n");
            return -1;
        }
        if (!hb_validate_filter_preset(HB_FILTER_DEINTERLACE,
                                       deinterlace, NULL))
        {
            // Nothing to do, but must validate preset before
            // attempting to validate custom settings to prevent potential
            // false positive
        }
        else if (!hb_validate_filter_settings(HB_FILTER_DEINTERLACE, deinterlace))
        {
            deinterlace_custom = 1;
        }
        else
        {
            fprintf(stderr, "Invalid deinterlace option %s\n", deinterlace);
            return -1;
        }
    }

    if (decomb != NULL)
    {
        if (decomb_disable)
        {
            fprintf(stderr,
                    "Incompatible options --decomb and --no-decomb\n");
            return -1;
        }
        if (!hb_validate_filter_preset(HB_FILTER_DECOMB, decomb, NULL))
        {
            // Nothing to do, but must validate preset before
            // attempting to validate custom settings to prevent potential
            // false positive
        }
        else if (!hb_validate_filter_settings(HB_FILTER_DECOMB, decomb))
        {
            decomb_custom = 1;
        }
        else
        {
            fprintf(stderr, "Invalid decomb option %s\n", decomb);
            return -1;
        }
    }

    if (hqdn3d != NULL)
    {
        if (hqdn3d_disable)
        {
            fprintf(stderr,
                    "Incompatible options --hqdn3d and --no-hqdn3d\n");
            return -1;
        }
        if (!hb_validate_filter_preset(HB_FILTER_HQDN3D, hqdn3d, NULL))
        {
            // Nothing to do, but must validate preset before
            // attempting to validate custom settings to prevent potential
            // false positive
        }
        else if (!hb_validate_filter_settings(HB_FILTER_HQDN3D, hqdn3d))
        {
            hqdn3d_custom = 1;
        }
        else
        {
            fprintf(stderr, "Invalid hqdn3d option %s\n", hqdn3d);
            return -1;
        }
    }

    if (nlmeans != NULL)
    {
        if (nlmeans_disable)
        {
            fprintf(stderr,
                    "Incompatible options --nlmeans and --no-nlmeans\n");
            return -1;
        }
        if (!hb_validate_filter_preset(HB_FILTER_NLMEANS, nlmeans, nlmeans_tune))
        {
            // Nothing to do, but must validate preset before
            // attempting to validate custom settings to prevent potential
            // false positive
        }
        else if (!hb_validate_filter_settings(HB_FILTER_NLMEANS, nlmeans))
        {
            nlmeans_custom = 1;
        }
        else
        {
            fprintf(stderr, "Invalid nlmeans option %s\n", nlmeans);
            return -1;
        }
    }

    return 0;
}

static int foreign_audio_scan(char **subtracks)
{
    if (subtracks != NULL)
    {
        int count = str_vlen(subtracks);
        int ii;
        for (ii = 0; ii < count; ii++)
        {
            if (!strcasecmp(subtracks[0], "scan"))
            {
                return 1;
            }
        }
    }
    return 0;
}

static int count_subtitles(char **subtracks)
{
    int subtitle_track_count = 0;
    if (subtracks != NULL)
    {
        int count = str_vlen(subtracks);
        int ii;
        for (ii = 0; ii < count; ii++)
        {
            if (strcasecmp(subtracks[0], "scan") &&
                strcasecmp(subtracks[0], "none"))
            {
                subtitle_track_count++;
            }
        }
    }
    return subtitle_track_count;
}

static int CheckOptions( int argc, char ** argv )
{
    if( update )
    {
        return 0;
    }

    if (preset_export_name == NULL && (input == NULL || *input == '\0'))
    {
        fprintf( stderr, "Missing input device. Run %s --help for "
                 "syntax.\n", argv[0] );
        return 1;
    }

    /* Parse format */
    if (titleindex > 0 && !titlescan)
    {
        if (preset_export_name == NULL && (output == NULL || *output == '\0'))
        {
            fprintf( stderr, "Missing output file name. Run %s --help "
                     "for syntax.\n", argv[0] );
            return 1;
        }

        if (format == NULL && output != NULL)
        {
            /* autodetect */
            const char *extension = strrchr(output, '.');
            if (extension != NULL)
            {
                // skip '.'
                mux = hb_container_get_from_extension(extension + 1);
            }
            hb_container_t * c = hb_container_get_from_format(mux);
            if (c != NULL)
                format = strdup(c->short_name);
        }
    }

    int subtitle_track_count = count_subtitles(subtracks);
    if (subtitle_track_count > 0 && subtitle_lang_list != NULL)
    {
        fprintf(stderr,
                "Incompatible options: --subtitle-lang-list and --subtitle\n");
        return 1;
    }

    if (subtitle_track_count > 0 && subtitle_all != -1)
    {
        fprintf(stderr,
                "Incompatible options: --all-subtitles/--first-subtitle and --subtitle\n");
        return 1;
    }

    if (atracks != NULL && audio_lang_list != NULL)
    {
        fprintf(stderr,
                "Incompatible options: --audio-lang-list and --audio\n");
        return 1;
    }

    if (atracks != NULL && audio_all != -1)
    {
        fprintf(stderr,
                "Incompatible options: --all-audio/--first-audio and --audio\n");
        return 1;
    }

    if ((par_width > 0 && par_height > 0) && display_width > 0)
    {
        fprintf(stderr,
                "Incompatible options: --display-width and --pixel-aspect\n");
        return 1;
    }

    if (preset_export_file != NULL && preset_export_name == NULL)
    {
        fprintf(stderr,
                "Error: --preset-export-file requires option --preset-export\n");
        return 1;
    }

    if (preset_export_desc != NULL && preset_export_name == NULL)
    {
        fprintf(stderr,
                "Error: --preset-export-desc requires option --preset-export\n");
        return 1;
    }

    return 0;
}

static hb_dict_t * PreparePreset(const char *preset_name)
{
    int ii;
    hb_dict_t *preset;

    if (preset_name != NULL)
    {
        preset = hb_preset_search(preset_name, 1 /*recurse*/);
        if (preset == NULL)
        {
            fprintf(stderr, "Invalid preset %s\n"
                            "Valid presets are:\n", preset_name);
            ShowPresets(NULL, 1, 1);
            return NULL;
        }
    }
    else
    {
        preset = hb_presets_get_default();
    }
    if (preset == NULL)
    {
        fprintf(stderr, "Error loading presets! Aborting.\n");
        return NULL;
    }
    preset = hb_value_dup(preset);

    int subtitle_track_count = count_subtitles(subtracks);
    // Apply any overrides that can be made directly to the preset
    if (format != NULL)
    {
        hb_dict_set(preset, "FileFormat", hb_value_string(format));
    }
    if (mp4_optimize != -1)
    {
        hb_dict_set(preset, "Mp4HttpOptimize", hb_value_bool(mp4_optimize));
    }
    if (ipod_atom != -1)
    {
        hb_dict_set(preset, "Mp4iPodCompatible", hb_value_bool(ipod_atom));
    }
    if (chapter_markers != -1)
    {
        hb_dict_set(preset, "ChapterMarkers", hb_value_bool(chapter_markers));
    }
    hb_value_array_t *subtitle_lang_array;
    subtitle_lang_array = hb_dict_get(preset, "SubtitleLanguageList");
    if (subtitle_lang_array == NULL)
    {
        subtitle_lang_array = hb_value_array_init();
        hb_dict_set(preset, "SubtitleLanguageList", subtitle_lang_array);
    }
    if (subtitle_lang_list != NULL)
    {
        hb_value_array_clear(subtitle_lang_array);
        int count = str_vlen(subtitle_lang_list);
        for (ii = 0; ii < count; ii++)
        {
            const iso639_lang_t *lang = lang_lookup(subtitle_lang_list[ii]);
            if (lang != NULL)
            {
                hb_value_array_append(subtitle_lang_array,
                                      hb_value_string(lang->iso639_2));
            }
            else
            {
                fprintf(stderr, "Warning: Invalid subtitle language (%s)\n",
                        subtitle_lang_list[ii]);
                return NULL;
            }
        }
        hb_dict_set(preset, "SubtitleTrackSelectionBehavior",
                    hb_value_string("first"));
    }
    if (native_language != NULL)
    {
        // Add native language subtitles if audio is not native
        lang_list_remove(subtitle_lang_array, native_language);
        hb_value_array_insert(subtitle_lang_array, 0,
                              hb_value_string(native_language));
        hb_dict_set(preset, "SubtitleAddForeignAudioSubtitle",
                    hb_value_bool(1));
    }
    if (foreign_audio_scan(subtracks))
    {
        // Add foreign audio search
        hb_dict_set(preset, "SubtitleAddForeignAudioSearch", hb_value_bool(1));
    }
    // Subtitle burn behavior
    const char *burn = "none";
    if (subtitle_track_count == 0)
    {
        if (subburn_native && subburn == 1)
        {
            burn = "foreign_first";
        }
        else if (subburn_native)
        {
            burn = "foreign";
        }
        else if (subburn == 1)
        {
            burn = "first";
        }
    }
    else
    {
        if (subburn_native)
        {
            burn = "foreign";
        }
    }
    hb_dict_set(preset, "SubtitleBurnBehavior", hb_value_string(burn));
    const char *selection = NULL;
    if (subtitle_track_count == 0 && subtitle_all != -1)
    {
        selection = subtitle_all == 1 ? "all" : "first";
    }
    else
    {
        selection = "none";
    }
    if (selection != NULL)
    {
        hb_dict_set(preset, "SubtitleTrackSelectionBehavior",
                    hb_value_string(selection));
    }

    if (audio_copy_list != NULL)
    {
        // Create autopassthru copy mask
        hb_value_array_t *array = hb_value_array_init();
        for (ii = 0; audio_copy_list[ii] != NULL; ii++)
        {
            hb_value_array_append(array, hb_value_string(audio_copy_list[ii]));
        }
        hb_dict_set(preset, "AudioCopyMask", array);
    }
    if (acodec_fallback != NULL)
    {
        hb_dict_set(preset, "AudioEncoderFallback",
                    hb_value_string(acodec_fallback));
    }

    hb_value_array_t *audio_lang_array;
    audio_lang_array = hb_dict_get(preset, "AudioLanguageList");
    if (audio_lang_array == NULL)
    {
        audio_lang_array = hb_value_array_init();
        hb_dict_set(preset, "AudioLanguageList", audio_lang_array);
    }
    if (audio_lang_list != NULL)
    {
        hb_value_array_clear(audio_lang_array);
        int count = str_vlen(audio_lang_list);
        for (ii = 0; ii < count; ii++)
        {
            const iso639_lang_t *lang = lang_lookup(audio_lang_list[ii]);
            if (lang != NULL)
            {
                hb_value_array_append(audio_lang_array,
                                      hb_value_string(lang->iso639_2));
            }
            else
            {
                fprintf(stderr, "Warning: Invalid audio language (%s)\n",
                        audio_lang_list[ii]);
                return NULL;
            }
        }
        hb_dict_set(preset, "AudioTrackSelectionBehavior",
                    hb_value_string("first"));
    }
    if (native_dub && native_language)
    {
        // Add native language audio
        lang_list_remove(audio_lang_array, native_language);
        hb_value_array_insert(audio_lang_array, 0,
                              hb_value_string(native_language));
    }
    if (audio_all != -1)
    {
        hb_dict_set(preset, "AudioTrackSelectionBehavior",
                    hb_value_string(audio_all == 1 ? "all" : "first"));
    }

    // Audio overrides
    if (atracks == NULL && (
        acodecs                   != NULL ||
        abitrates                 != NULL ||
        arates                    != NULL ||
        mixdowns                  != NULL ||
        normalize_mix_level       != NULL ||
        audio_dither              != NULL ||
        dynamic_range_compression != NULL ||
        audio_gain                != NULL ||
        aqualities                != NULL ||
        acompressions             != NULL ||
        anames                    != NULL))
    {
        // No explicit audio tracks, but track settings modified.
        // Modify the presets audio settings.
        hb_value_array_t *list;
        list = hb_dict_get(preset, "AudioList");
        if (list == NULL)
        {
            list = hb_value_array_init();
            hb_dict_set(preset, "AudioList", list);
        }
        int count = MAX(str_vlen(mixdowns),
                    MAX(str_vlen(dynamic_range_compression),
                    MAX(str_vlen(audio_gain),
                    MAX(str_vlen(audio_dither),
                    MAX(str_vlen(normalize_mix_level),
                    MAX(str_vlen(arates),
                    MAX(str_vlen(abitrates),
                    MAX(str_vlen(aqualities),
                    MAX(str_vlen(acompressions),
                    MAX(str_vlen(acodecs),
                        str_vlen(anames)))))))))));

        hb_dict_t *audio_dict;
        // Add audio dict entries to list if needed
        for (ii = 0; ii < count; ii++)
        {
            audio_dict = hb_value_array_get(list, ii);
            if (audio_dict == NULL)
            {
                audio_dict = hb_dict_init();
                hb_value_array_append(list, audio_dict);
            }
        }

        // Update codecs
        if (str_vlen(acodecs) > 0)
        {
            for (ii = 0; acodecs[ii] != NULL; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioEncoder",
                                    hb_value_string(acodecs[ii]));
            }
            // Apply last codec in list to all other entries
            for (; ii < count; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioEncoder",
                                    hb_value_string(acodecs[ii-1]));
            }
        }

        // Update qualities
        if (str_vlen(aqualities) > 0)
        {
            for (ii = 0; aqualities[ii] != NULL &&
                         aqualities[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioTrackQualityEnable",
                            hb_value_bool(1));
                hb_dict_set(audio_dict, "AudioTrackQuality",
                    hb_value_double(strtod(aqualities[ii], NULL)));
            }
        }

        // Update bitrates
        if (str_vlen(abitrates) > 0)
        {
            for (ii = 0; abitrates[ii]    != NULL &&
                         abitrates[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                if (hb_value_get_bool(hb_dict_get(audio_dict,
                                      "AudioTrackQualityEnable")))
                {
                    continue;
                }
                hb_dict_set(audio_dict, "AudioBitrate",
                    hb_value_int(atoi(abitrates[ii])));
            }
            // Apply last bitrate in list to all other entries
            if (abitrates[ii-1][0] != 0)
                for (; ii < count; ii++)
                {
                    audio_dict = hb_value_array_get(list, ii);
                    if (hb_value_get_bool(hb_dict_get(audio_dict,
                                        "AudioTrackQualityEnable")))
                    {
                        continue;
                    }
                    hb_dict_set(audio_dict, "AudioBitrate",
                        hb_value_int(atoi(abitrates[ii-1])));
                }
        }

        // Update samplerates
        if (str_vlen(arates) > 0)
        {
            for (ii = 0; arates[ii]    != NULL &&
                         arates[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioSamplerate",
                            hb_value_string(arates[ii]));
            }
            // Apply last samplerate in list to all other entries
            if (arates[ii-1][0] != 0)
                for (; ii < count; ii++)
                {
                    audio_dict = hb_value_array_get(list, ii);
                    hb_dict_set(audio_dict, "AudioSamplerate",
                                hb_value_string(arates[ii-1]));
                }
        }

        // Update mixdowns
        if (str_vlen(mixdowns) > 0)
        {
            for (ii = 0; mixdowns[ii]    != NULL &&
                         mixdowns[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioMixdown",
                            hb_value_string(mixdowns[ii]));
            }
            // Apply last codec in list to all other entries
            if (mixdowns[ii-1][0] != 0)
                for (; ii < count; ii++)
                {
                    audio_dict = hb_value_array_get(list, ii);
                    hb_dict_set(audio_dict, "AudioMixdown",
                                hb_value_string(mixdowns[ii-1]));
                }
        }

        // Update mixdowns normalization
        if (str_vlen(normalize_mix_level) > 0)
        {
            for (ii = 0; normalize_mix_level[ii]    != NULL &&
                         normalize_mix_level[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioNormalizeMixLevel",
                    hb_value_bool(atoi(normalize_mix_level[ii])));
            }
            // Apply last mix norm in list to all other entries
            if (normalize_mix_level[ii-1][0] != 0)
                for (; ii < count; ii++)
                {
                    audio_dict = hb_value_array_get(list, ii);
                    hb_dict_set(audio_dict,
                                "AudioNormalizeMixLevel",
                        hb_value_bool(
                            atoi(normalize_mix_level[ii-1])));
                }
        }

        // Update DRC
        if (str_vlen(dynamic_range_compression) > 0)
        {
            for (ii = 0;dynamic_range_compression[ii]    != NULL &&
                        dynamic_range_compression[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioTrackDRCSlider",
                  hb_value_double(
                    strtod(dynamic_range_compression[ii], NULL)));
            }
            // Apply last DRC in list to all other entries
            if (dynamic_range_compression[ii-1][0] != 0)
                for (; ii < count; ii++)
                {
                    audio_dict = hb_value_array_get(list, ii);
                    hb_dict_set(audio_dict, "AudioTrackDRCSlider",
                        hb_value_double(
                            strtod(dynamic_range_compression[ii-1],
                                   NULL)));
                }
        }

        // Update Gain
        if (str_vlen(audio_gain) > 0)
        {
            for (ii = 0; audio_gain[ii]    != NULL &&
                         audio_gain[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioTrackGainSlider",
                  hb_value_double(
                    strtod(audio_gain[ii], NULL)));
            }
            // Apply last gain in list to all other entries
            if (audio_gain[ii-1][0] != 0)
                for (; ii < count; ii++)
                {
                    audio_dict = hb_value_array_get(list, ii);
                    hb_dict_set(audio_dict, "AudioTrackGainSlider",
                      hb_value_double(
                        strtod(audio_gain[ii-1], NULL)));
                }
        }

        // Update dither method
        if (str_vlen(audio_dither) > 0)
        {
            for (ii = 0; audio_dither[ii]    != NULL &&
                         audio_dither[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioDitherMethod",
                            hb_value_string(audio_dither[ii]));
            }
            // Apply last dither in list to all other entries
            if (audio_dither[ii-1][0] != 0)
                for (; ii < count; ii++)
                {
                    audio_dict = hb_value_array_get(list, ii);
                    hb_dict_set(audio_dict, "AudioDitherMethod",
                            hb_value_string(audio_dither[ii-1]));
                }
        }

        // Update compression
        if (str_vlen(acompressions) > 0)
        {
            for (ii = 0; acompressions[ii]    != NULL &&
                         acompressions[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioCompressionLevel",
                  hb_value_double(
                    strtod(acompressions[ii], NULL)));
            }
            // Apply last compression in list to all other entries
            if (acompressions[ii-1][0] != 0)
                for (; ii < count; ii++)
                {
                    audio_dict = hb_value_array_get(list, ii);
                    hb_dict_set(audio_dict, "AudioCompressionLevel",
                      hb_value_double(
                        strtod(acompressions[ii-1], NULL)));
                }
        }

        // Update track names
        if (str_vlen(anames) > 0)
        {
            for (ii = 0; anames[ii]    != NULL &&
                         anames[ii][0] != 0; ii++)
            {
                audio_dict = hb_value_array_get(list, ii);
                hb_dict_set(audio_dict, "AudioTrackName",
                                    hb_value_string(acodecs[ii]));
            }
        }
    }

    if (atracks != NULL)
    {
        // User has specified explicit audio tracks
        // Disable preset's audio track selection
        hb_dict_set(preset, "AudioTrackSelectionBehavior",
                    hb_value_string("none"));
    }

    if (vcodec != NULL)
    {
        const char *s;
        int old, new;

        s = hb_value_get_string(hb_dict_get(preset, "VideoEncoder"));
        old = hb_video_encoder_get_from_name(s);
        new = hb_video_encoder_get_from_name(vcodec);
        if (old != new)
        {
            // If the user explicitly changes a video encoder, remove the
            // preset VideoPreset, VideoTune, VideoProfile, VideoLevel, and
            // VideoOptionExtra.
            //
            // Use defaults for the encoder since these settings may not be
            // compatible across encoders.
            hb_dict_remove(preset, "VideoPreset");
            hb_dict_remove(preset, "VideoTune");
            hb_dict_remove(preset, "VideoProfile");
            hb_dict_remove(preset, "VideoLevel");
            hb_dict_remove(preset, "VideoOptionExtra");
        }
        hb_dict_set(preset, "VideoEncoder", hb_value_string(vcodec));
    }
    if (encoder_preset != NULL)
    {
        hb_dict_set(preset, "VideoPreset", hb_value_string(encoder_preset));
    }
    if (encoder_tune != NULL)
    {
        hb_dict_set(preset, "VideoTune", hb_value_string(encoder_tune));
    }
    if (encoder_profile != NULL)
    {
        hb_dict_set(preset, "VideoProfile", hb_value_string(encoder_profile));
    }
    if (encoder_level != NULL)
    {
        hb_dict_set(preset, "VideoLevel", hb_value_string(encoder_level));
    }
    if (advanced_opts != NULL)
    {
        hb_dict_set(preset, "VideoOptionExtra", hb_value_string(advanced_opts));
    }
    if (vquality >= 0)
    {
        hb_dict_set(preset, "VideoQualityType", hb_value_int(2));
        hb_dict_set(preset, "VideoQualitySlider", hb_value_double(vquality));
    }
    else if (vbitrate != 0)
    {
        hb_dict_set(preset, "VideoQualityType", hb_value_int(1));
        hb_dict_set(preset, "VideoAvgBitrate", hb_value_int(vbitrate));
        if (twoPass)
        {
            hb_dict_set(preset, "VideoTwoPass", hb_value_bool(1));
        }
        if (fastfirstpass)
        {
            hb_dict_set(preset, "VideoTurboTwoPass", hb_value_bool(1));
        }
    }
    if (vrate != NULL)
    {
        hb_dict_set(preset, "VideoFramerate", hb_value_string(vrate));
    }
    else
    {
        hb_dict_set(preset, "VideoFramerate", hb_value_string("auto"));
    }
    if (cfr != -1)
    {
        hb_dict_set(preset, "VideoFramerateMode",
                    hb_value_string(cfr == 0 ? "vfr" :
                                    cfr == 1 ? "cfr" : "pfr"));
    }
    if (color_matrix_code > 0)
    {
        hb_dict_set(preset, "VideoColorMatrixCode",
                    hb_value_int(color_matrix_code));
    }
#ifdef USE_QSV
    if (qsv_async_depth >= 0)
    {
        hb_dict_set(preset, "VideoQSVAsyncDepth",
                        hb_value_int(qsv_async_depth));
    }
    if (qsv_decode != -1)
    {
        hb_dict_set(preset, "VideoQSVDecode", hb_value_int(qsv_decode));
    }
#endif
    if (use_hwd != -1)
    {
        hb_dict_set(preset, "VideoHWDecode", hb_value_bool(use_hwd));
    }
    if (use_opencl != -1)
    {
        hb_dict_set(preset, "VideoScaler",
                    hb_value_string(use_opencl ? "opencl" : "swscale"));
    }
    if (maxWidth > 0)
    {
        hb_dict_set(preset, "PictureWidth", hb_value_int(maxWidth));
    }
    if (maxHeight > 0)
    {
        hb_dict_set(preset, "PictureHeight", hb_value_int(maxHeight));
    }
    if (width > 0)
    {
        hb_dict_set(preset, "PictureForceWidth", hb_value_int(width));
    }
    if (height > 0)
    {
        hb_dict_set(preset, "PictureForceHeight", hb_value_int(height));
    }
    if (crop[0] >= 0 || crop[1] >= 0 || crop[2] >= 0 || crop[3] >= 0)
    {
        hb_dict_set(preset, "PictureAutoCrop", hb_value_bool(0));
    }
    if (crop[0] >= 0)
    {
        hb_dict_set(preset, "PictureTopCrop", hb_value_int(crop[0]));
    }
    if (crop[1] >= 0)
    {
        hb_dict_set(preset, "PictureBottomCrop", hb_value_int(crop[1]));
    }
    if (crop[2] >= 0)
    {
        hb_dict_set(preset, "PictureLeftCrop", hb_value_int(crop[2]));
    }
    if (crop[3] >= 0)
    {
        hb_dict_set(preset, "PictureRightCrop", hb_value_int(crop[3]));
    }
    if (loose_crop != -1)
    {
        hb_dict_set(preset, "PictureLooseCrop", hb_value_bool(loose_crop));
    }
    if (display_width > 0)
    {
        keep_display_aspect = 0;
        anamorphic_mode = 3;
        hb_dict_set(preset, "PictureDARWidth", hb_value_int(display_width));
    }
    else if (par_width > 0 && par_height > 0)
    {
        keep_display_aspect = 0;
        anamorphic_mode = 3;
        hb_dict_set(preset, "PicturePARWidth", hb_value_int(par_width));
        hb_dict_set(preset, "PicturePARHeight", hb_value_int(par_height));
    }
    if (anamorphic_mode != -1)
    {
        hb_dict_set(preset, "PicturePAR", hb_value_int(anamorphic_mode));
    }
    if (keep_display_aspect != -1)
    {
        hb_dict_set(preset, "PictureKeepRatio",
                    hb_value_bool(keep_display_aspect));
    }
    if (itu_par != -1)
    {
        hb_dict_set(preset, "PictureItuPAR", hb_value_bool(itu_par));
    }
    if (modulus > 0)
    {
        hb_dict_set(preset, "PictureModulus", hb_value_int(modulus));
    }
    if (grayscale != -1)
    {
        hb_dict_set(preset, "VideoGrayScale", hb_value_bool(grayscale));
    }
    if (decomb_disable || deinterlace_disable)
    {
        hb_dict_set(preset, "PictureDeinterlaceFilter", hb_value_string("off"));
    }
    if (deinterlace != NULL)
    {
        hb_dict_set(preset, "PictureDeinterlaceFilter",
                    hb_value_string("deinterlace"));
        if (!deinterlace_custom)
        {
            hb_dict_set(preset, "PictureDeinterlacePreset",
                        hb_value_string(deinterlace));
        }
        else
        {
            hb_dict_set(preset, "PictureDeinterlacePreset",
                        hb_value_string("custom"));
            hb_dict_set(preset, "PictureDeinterlaceCustom",
                        hb_value_string(deinterlace));
        }
    }
    if (decomb != NULL)
    {
        hb_dict_set(preset, "PictureDeinterlaceFilter",
                    hb_value_string("decomb"));
        if (!decomb_custom)
        {
            hb_dict_set(preset, "PictureDeinterlacePreset",
                        hb_value_string(decomb));
        }
        else
        {
            hb_dict_set(preset, "PictureDeinterlacePreset",
                        hb_value_string("custom"));
            hb_dict_set(preset, "PictureDeinterlaceCustom",
                        hb_value_string(decomb));
        }
    }
    if (detelecine_disable)
    {
        hb_dict_set(preset, "PictureDetelecine", hb_value_string("off"));
    }
    if (detelecine != NULL)
    {
        if (!detelecine_custom)
        {
            hb_dict_set(preset, "PictureDetelecine",
                        hb_value_string(detelecine));
        }
        else
        {
            hb_dict_set(preset, "PictureDetelecine", hb_value_string("custom"));
            hb_dict_set(preset, "PictureDetelecineCustom",
                        hb_value_string(detelecine));
        }
    }
    const char *s;
    s = hb_value_get_string(hb_dict_get(preset, "PictureDenoiseFilter"));
    if (hqdn3d_disable && !strcasecmp(s, "hqdn3d"))
    {
        hb_dict_set(preset, "PictureDenoiseFilter", hb_value_string("off"));
    }
    if (hqdn3d != NULL)
    {
        hb_dict_set(preset, "PictureDenoiseFilter", hb_value_string("hqdn3d"));
        if (!hqdn3d_custom)
        {
            hb_dict_set(preset, "PictureDenoisePreset",
                        hb_value_string(hqdn3d));
        }
        else
        {
            hb_dict_set(preset, "PictureDenoisePreset",
                        hb_value_string("custom"));
            hb_dict_set(preset, "PictureDenoiseCustom",
                        hb_value_string(hqdn3d));
        }
    }
    if (nlmeans_disable && !strcasecmp(s, "nlmeans"))
    {
        hb_dict_set(preset, "PictureDenoiseFilter", hb_value_string("off"));
    }
    if (nlmeans != NULL)
    {
        hb_dict_set(preset, "PictureDenoiseFilter", hb_value_string("nlmeans"));
        if (!nlmeans_custom)
        {
            hb_dict_set(preset, "PictureDenoisePreset",
                        hb_value_string(nlmeans));
            if (nlmeans_tune != NULL)
            {
                hb_dict_set(preset, "PictureDenoiseTune",
                            hb_value_string(nlmeans_tune));
            }
        }
        else
        {
            hb_dict_set(preset, "PictureDenoisePreset",
                        hb_value_string("custom"));
            hb_dict_set(preset, "PictureDenoiseCustom",
                        hb_value_string(nlmeans));
        }
    }
    if (deblock_disable)
    {
        hb_dict_set(preset, "PictureDeblock", hb_value_string("0"));
    }
    if (deblock != NULL)
    {
        hb_dict_set(preset, "PictureDeblock", hb_value_string(deblock));
    }
    if (rotate != NULL)
    {
        hb_dict_set(preset, "PictureRotate", hb_value_string(rotate));
    }

    return preset;
}


static int add_sub(hb_value_array_t *list, hb_title_t *title, int track, int *one_burned)
{
    hb_subtitle_t *subtitle;
    // Check that the track exists
    subtitle = hb_list_item(title->list_subtitle, track);
    if (subtitle == NULL)
    {
        fprintf(stderr, "Warning: Could not find subtitle track %d, skipped\n",
                track + 1);
        return -1;
    }

    int out_track = hb_value_array_len(list);
    int burn = !*one_burned && subburn == out_track + 1 &&
               hb_subtitle_can_burn(subtitle->source);
    *one_burned |= burn;
    int def  = subdefault == out_track + 1;
    int force = test_sub_list(subforce, out_track + 1);

    if (!burn &&
        !hb_subtitle_can_pass(subtitle->source, mux))
    {
        // Only allow one subtitle to be burned into video
        if (*one_burned)
        {
            fprintf(stderr, "Warning: Skipping subtitle track %d, can't have more than one track burnt in\n", track + 1);
            return -1;
        }
        *one_burned = 1;
    }
    hb_dict_t *subtitle_dict = hb_dict_init();
    hb_dict_set(subtitle_dict, "Track", hb_value_int(track));
    hb_dict_set(subtitle_dict, "Default", hb_value_bool(def));
    hb_dict_set(subtitle_dict, "Forced", hb_value_bool(force));
    hb_dict_set(subtitle_dict, "Burn", hb_value_bool(burn));
    hb_value_array_append(list, subtitle_dict);
    return 0;
}

static int add_srt(hb_value_array_t *list, int track, int *one_burned)
{
    char *codeset = "ISO-8859-1";
    int64_t offset = 0;
    char *iso639_2 = "und";
    int burn = !*one_burned && srtburn == track + 1 &&
               hb_subtitle_can_burn(SRTSUB);
    *one_burned |= burn;
    int def  = srtdefault == track + 1;

    if (srtcodeset && track < str_vlen(srtcodeset) && srtcodeset[track])
        codeset = srtcodeset[track];
    if (srtoffset && track < str_vlen(srtoffset) && srtoffset[track])
        offset = strtoll(srtoffset[track], NULL, 0);
    if (srtlang && track < str_vlen(srtlang) && srtlang[track])
    {
        const iso639_lang_t *lang = lang_lookup(srtlang[track]);
        if (lang != NULL)
        {
            iso639_2 = lang->iso639_2;
        }
        else
        {
            fprintf(stderr, "Warning: Invalid SRT language (%s)\n",
                    srtlang[track]);
        }
    }

    hb_dict_t *subtitle_dict = hb_dict_init();
    hb_dict_t *srt_dict = hb_dict_init();
    hb_dict_set(subtitle_dict, "SRT", srt_dict);
    hb_dict_set(subtitle_dict, "Default", hb_value_bool(def));
    hb_dict_set(subtitle_dict, "Burn", hb_value_bool(burn));
    hb_dict_set(subtitle_dict, "Offset", hb_value_int(offset));
    hb_dict_set(srt_dict, "Filename", hb_value_string(srtfile[track]));
    hb_dict_set(srt_dict, "Language", hb_value_string(iso639_2));
    hb_dict_set(srt_dict, "Codeset", hb_value_string(codeset));
    hb_value_array_append(list, subtitle_dict);
    return 0;
}

static int add_audio(hb_value_array_t *list, hb_title_t *title, int track)
{
    // Check that the track exists
    if (hb_list_item(title->list_audio, track-1) == NULL)
    {
        fprintf(stderr, "Warning: Could not find audio track %d, skipped\n",
                track);
        return -1;
    }
    hb_dict_t *audio_dict = hb_dict_init();
    hb_dict_set(audio_dict, "Track", hb_value_int(track-1));
    hb_value_array_append(list, audio_dict);

    return 0;
}

static hb_dict_t*
PrepareJob(hb_handle_t *h, hb_title_t *title, hb_dict_t *preset_dict)
{
    hb_dict_t *job_dict;
    job_dict = hb_preset_job_init(h, title->index, preset_dict);
    if (job_dict == NULL)
    {
        fprintf(stderr, "Failed to initialize job\n");
        return NULL;
    }

    if (hb_value_get_bool(hb_dict_get(job_dict, "ChapterMarkers")))
    {
        write_chapter_names(job_dict, marker_file);
    }

    hb_dict_t *dest_dict = hb_dict_get(job_dict, "Destination");
    hb_dict_set(dest_dict, "File", hb_value_string(output));

    // Now that the job is initialized, we need to find out
    // what muxer is being used.
    mux = hb_container_get_from_name(
        hb_value_get_string(hb_dict_get(dest_dict, "Mux")));

    // Now set non-preset settings in the job dict
    int range_start = 0, range_end = 0, range_seek_points = 0;
    const char *range_type = "chapter";
    if (chapter_start   && chapter_end    &&
        !start_at_pts   && !stop_at_pts   &&
        !start_at_frame && !stop_at_frame &&
        !start_at_preview )
    {
        range_type = "chapter";
        int chapter_count = hb_list_count(title->list_chapter);
        range_start = MAX(1, chapter_start);
        range_end   = MAX(chapter_start, MIN(chapter_count, chapter_end));
    }
    else if (start_at_preview)
    {
        range_type = "preview";
        range_start = start_at_preview -1;
        range_end = stop_at_pts;
        range_seek_points = preview_count;
    }
    else if (start_at_pts || stop_at_pts)
    {
        range_type = "time";
        range_start = start_at_pts;
        range_end = stop_at_pts;
    }
    else if (start_at_frame || stop_at_frame)
    {
        range_type = "frame";
        range_start = start_at_frame;
        range_end = stop_at_frame;
    }
    if (range_start || range_end)
    {
        hb_dict_t *range_dict = hb_dict_get(
                            hb_dict_get(job_dict, "Source"), "Range");
        hb_dict_set(range_dict, "Type", hb_value_string(range_type));
        if (range_start)
            hb_dict_set(range_dict, "Start", hb_value_int(range_start));
        else
            hb_dict_remove(range_dict, "Start");
        if (range_end)
            hb_dict_set(range_dict, "End",   hb_value_int(range_end));
        else
            hb_dict_remove(range_dict, "End");
        if (range_seek_points)
            hb_dict_set(range_dict, "SeekPoints",
                    hb_value_int(range_seek_points));
    }

    if (angle)
    {
        hb_dict_t *source_dict = hb_dict_get(job_dict, "Source");
        hb_dict_set(source_dict, "Angle", hb_value_int(angle));
    }

    hb_dict_t *subtitles_dict = hb_dict_get(job_dict, "Subtitle");
    hb_value_array_t * subtitle_array;
    subtitle_array = hb_dict_get(subtitles_dict, "SubtitleList");

    hb_dict_t *audios_dict = hb_dict_get(job_dict, "Audio");
    hb_value_array_t * audio_array = hb_dict_get(audios_dict, "AudioList");
    hb_dict_t *audio_dict;

    /* Grab audio tracks */
    if (atracks != NULL)
    {
        int track_count;
        int ii;
        if (atracks[0] != NULL && strcasecmp("none", atracks[0]))
        {
            // First "track" is not "none", add tracks
            for (ii = 0; atracks[ii] != NULL; ii++)
            {
                int first, last, track;
                if (sscanf(atracks[ii], "%d-%d", &first, &last ) == 2)
                {
                    for (track = first - 1; track < last; track++)
                    {
                        add_audio(audio_array, title, track);
                    }
                }
                else if (sscanf(atracks[ii], "%d", &track) == 1)
                {
                    add_audio(audio_array, title, track);
                }
                else
                {
                    fprintf(stderr, "ERROR: unable to parse audio input \"%s\", skipping\n", atracks[ii]);
                }
            }
        }
        track_count = hb_value_array_len(audio_array);

        // Now we need to take care of initializing subtitle selection
        // for foreign audio since this could not be done by the preset
        // due to disabling of preset audio selection.

        // Determine the language of the first audio track
        if (track_count > 0)
        {
            audio_dict = hb_value_array_get(audio_array, 0);
            int track = hb_value_get_int(hb_dict_get(audio_dict, "Track"));

            hb_audio_config_t *audio;
            audio = hb_list_audio_config_item(title->list_audio, track);
            if (audio != NULL)
            {
                hb_preset_job_add_subtitles(h, title->index,
                                            preset_dict, job_dict);
            }
        }

        /* Audio Codecs */
        int acodec = HB_ACODEC_INVALID;
        ii = 0;
        if (acodecs != NULL)
        {
            for (; acodecs[ii] != NULL && ii < track_count; ii++)
            {
                audio_dict = hb_value_array_get(audio_array, ii);
                acodec = hb_audio_encoder_get_from_name(acodecs[ii]);
                if (acodec <= 0)
                {
                    fprintf(stderr,
                        "Invalid codec %s, using default for container.\n",
                        acodecs[ii]);
                    acodec = hb_audio_encoder_get_default(mux);
                }
                hb_dict_set(audio_dict, "Encoder", hb_value_int(acodec));
            }
            if (acodecs[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio encoders\n");
            }
        }
        if (ii != 1)
        {
            acodec = hb_audio_encoder_get_default(mux);
        }
        for (; ii < track_count; ii++)
        {
            // We have fewer inputs than audio tracks, use the
            // default codec for this container for the remaining
            // tracks. Unless we only have one input then use that
            // codec instead.
            audio_dict = hb_value_array_get(audio_array, ii);
            hb_dict_set(audio_dict, "Encoder", hb_value_int(acodec));
        }

        /* Sample Rate */
        int arate = 0;
        ii = 0;
        if (arates != NULL)
        {
            for (; arates[ii] != NULL && ii < track_count; ii++)
            {
                if (!strcasecmp(arates[ii], "auto"))
                {
                    arate = 0;
                }
                else
                {
                    arate = hb_audio_samplerate_get_from_name(arates[ii]);
                }
                if (arate <= 0)
                {
                    fprintf(stderr, "Invalid sample rate %s, using input rate\n",
                            arates[ii]);
                    arate = 0;
                }
                audio_dict = hb_value_array_get(audio_array, ii);
                hb_dict_set(audio_dict, "Samplerate", hb_value_int(arate));
            }
            if (arates[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio sample rates\n");
            }
        }
        // If exactly one samplerate was specified, apply it to the reset
        // of the tracks.
        //
        // For any tracks that we do not set the rate for, libhb will
        // assign the source audio track's rate
        if (ii == 1) for (; ii < track_count; ii++)
        {
            audio_dict = hb_value_array_get(audio_array, ii);
            hb_dict_set(audio_dict, "Samplerate", hb_value_int(arate));
        }

        /* Audio Mixdown */
        int mix = HB_AMIXDOWN_NONE;
        ii = 0;
        if (mixdowns != NULL)
        {
            for (; mixdowns[ii] != NULL && ii < track_count; ii++)
            {
                mix = hb_mixdown_get_from_name(mixdowns[ii]);
                audio_dict = hb_value_array_get(audio_array, ii);
                hb_dict_set(audio_dict, "Mixdown", hb_value_int(mix));
            }
            if (mixdowns[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio mixdowns\n");
            }
        }
        // If exactly one mix was specified, apply it to the reset
        // of the tracks
        if (ii == 1) for (; ii < track_count; ii++)
        {
            audio_dict = hb_value_array_get(audio_array, ii);
            hb_dict_set(audio_dict, "Mixdown", hb_value_int(mix));
        }

        /* Audio Bitrate */
        int abitrate = 0;
        ii = 0;
        if (abitrates != NULL)
        {
            for (; abitrates[ii] != NULL && ii < track_count; ii++)
            {
                if (*abitrates[ii])
                {
                    abitrate = atoi(abitrates[ii]);
                    audio_dict = hb_value_array_get(audio_array, ii);
                    hb_dict_set(audio_dict, "Bitrate", hb_value_int(abitrate));
                }
            }
            if (abitrates[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio bitrates\n");
            }
        }
        // If exactly one bitrate was specified, apply it to the reset
        // of the tracks
        if (ii == 1) for (; ii < track_count; ii++)
        {
            audio_dict = hb_value_array_get(audio_array, ii);
            hb_dict_set(audio_dict, "Bitrate", hb_value_int(abitrate));
        }

        /* Audio Quality */
        double aquality = 0.;
        ii = 0;
        if (aqualities != NULL)
        {
            for (; aqualities[ii] != NULL && ii < track_count; ii++)
            {
                if (*aqualities[ii])
                {
                    aquality = atof(aqualities[ii]);
                    audio_dict = hb_value_array_get(audio_array, ii);
                    hb_dict_set(audio_dict, "Quality",
                                hb_value_double(aquality));
                    hb_dict_set(audio_dict, "Bitrate", hb_value_int(-1));
                }
            }
            if (aqualities[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio qualities\n");
            }
        }
        // If exactly one quality was specified, apply it to the reset
        // of the tracks that do not already have the bitrate set.
        if (ii == 1) for (; ii < track_count; ii++)
        {
            audio_dict = hb_value_array_get(audio_array, ii);
            abitrate = hb_value_get_int(hb_dict_get(audio_dict, "Bitrate"));
            if (abitrate <= 0)
            {
                hb_dict_set(audio_dict, "Quality", hb_value_double(aquality));
                hb_dict_set(audio_dict, "Bitrate", hb_value_int(-1));
            }
        }

        /* Audio Compression Level */
        double acompression;
        ii = 0;
        if (acompressions != NULL)
        {
            for (; acompressions[ii] != NULL && ii < track_count; ii++)
            {
                if (*acompressions[ii])
                {
                    acompression = atof(acompressions[ii]);
                    audio_dict = hb_value_array_get(audio_array, ii);
                    hb_dict_set(audio_dict, "CompressionLevel",
                                hb_value_double(acompression));
                }
            }
            if (acompressions[ii] != NULL)
            {
                fprintf(stderr,
                        "Dropping excess audio compression levels\n");
            }
        }
        // Compression levels are codec specific values.  So don't
        // try to apply to other tracks.

        /* Audio DRC */
        ii = 0;
        double drc = 0.;
        if (dynamic_range_compression)
        {
            char **drcs = dynamic_range_compression;
            for (; drcs[ii] != NULL && ii < track_count; ii++)
            {
                drc = atof(drcs[ii]);
                audio_dict = hb_value_array_get(audio_array, ii);
                hb_dict_set(audio_dict, "DRC", hb_value_double(drc));
            }
            if (drcs[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio dynamic range controls\n");
            }
        }
        // If exactly one DRC was specified, apply it to the reset
        // of the tracks
        if (ii == 1) for (; ii < track_count; ii++)
        {
            audio_dict = hb_value_array_get(audio_array, ii);
            hb_dict_set(audio_dict, "DRC", hb_value_double(drc));
        }

        /* Audio Gain */
        ii = 0;
        double gain = 1.;
        if (audio_gain)
        {
            for (; audio_gain[ii] != NULL && ii < track_count; ii++)
            {
                gain = atof(audio_gain[ii]);
                audio_dict = hb_value_array_get(audio_array, ii);
                hb_dict_set(audio_dict, "Gain", hb_value_double(gain));
            }
            if (audio_gain[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio gains\n");
            }
        }
        // If exactly one gain was specified, apply it to the reset
        // of the tracks
        if (ii == 1) for (; ii < track_count; ii++)
        {
            audio_dict = hb_value_array_get(audio_array, ii);
            hb_dict_set(audio_dict, "Gain", hb_value_double(gain));
        }

        /* Audio Dither */
        int dither = 0;
        ii = 0;
        if (audio_dither != NULL)
        {
            for (; audio_dither[ii] != NULL && ii < track_count; ii++)
            {
                if (*audio_dither[ii])
                {
                    dither = hb_audio_dither_get_from_name(audio_dither[ii]);
                    audio_dict = hb_value_array_get(audio_array, ii);
                    hb_dict_set(audio_dict, "DitherMethod",
                                hb_value_int(dither));
                }
            }
            if (audio_dither[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio dither methods\n");
            }
        }
        // If exactly one dither was specified, apply it to the reset
        // of the tracks
        if (ii == 1) for (; ii < track_count; ii++)
        {
            int codec;
            audio_dict = hb_value_array_get(audio_array, ii);
            codec = hb_value_get_int(hb_dict_get(audio_dict, "Encoder"));
            if (hb_audio_dither_is_supported(codec))
            {
                hb_dict_set(audio_dict, "DitherMethod",
                            hb_value_double(dither));
            }
        }

        /* Audio Mix Normalization */
        int norm = 0;
        ii = 0;
        if (normalize_mix_level)
        {
            char **nmls = normalize_mix_level;
            for (; nmls[ii] != NULL && ii < track_count; ii++)
            {
                norm = atoi(nmls[ii]);
                audio_dict = hb_value_array_get(audio_array, ii);
                hb_dict_set(audio_dict, "NormalizeMixLevel",
                            hb_value_int(norm));
            }
            if (nmls[ii] != NULL)
            {
                fprintf(stderr,
                        "Dropping excess audio mixdown normalizations\n");
            }
        }
        // If exactly one norm was specified, apply it to the reset
        // of the tracks
        if (ii == 1) for (; ii < track_count; ii++)
        {
            audio_dict = hb_value_array_get(audio_array, ii);
            hb_dict_set(audio_dict, "NormalizeMixLevel", hb_value_int(norm));
        }

        /* Audio Track Names */
        ii = 0;
        if (anames != NULL)
        {
            for (; anames[ii] != NULL && ii < track_count; ii++)
            {
                if (*anames[ii])
                {
                    audio_dict = hb_value_array_get(audio_array, ii);
                    hb_dict_set(audio_dict, "Name",
                                hb_value_string(anames[ii]));
                }
            }
            if (anames[ii] != NULL)
            {
                fprintf(stderr, "Dropping excess audio track names\n");
            }
        }
        // If exactly one name was specified, apply it to the reset
        // of the tracks
        if (ii == 1 && *anames[0]) for (; ii < track_count; ii++)
        {
            audio_dict = hb_value_array_get(audio_array, ii);
            hb_dict_set(audio_dict, "Name", hb_value_string(anames[0]));
        }
    }

    int one_burned = 0;
    if (subtracks != NULL)
    {
        int ii;
        for (ii = 0; subtracks[ii] != NULL; ii++)
        {
            if (strcasecmp(subtracks[ii], "none" ) == 0)
            {
                // Taken care of already when initializing the job
                // from a preset
                continue;
            }
            if (strcasecmp(subtracks[ii], "scan" ) == 0)
            {
                // Taken care of already when initializing the job
                // from a preset
                continue;
            }

            int first, last, track;
            if (sscanf(subtracks[ii], "%d-%d", &first, &last ) == 2)
            {
                for (track = first - 1; track < last; track++)
                {
                    add_sub(subtitle_array, title, track-1, &one_burned);
                }
            }
            else if (sscanf(subtracks[ii], "%d", &track) == 1)
            {
                add_sub(subtitle_array, title, track-1, &one_burned);
            }
            else
            {
                fprintf(stderr, "ERROR: unable to parse subtitle input \"%s\", skipping\n", subtracks[ii]);
            }
        }
    }
    else
    {
        if (subdefault > 0)
        {
            // "Default" flag can not be applied till after subtitles have
            // been selected.  Apply it here if subtitle selection was
            // made by the preset.
            hb_value_t *sub_dict = hb_dict_get(job_dict, "Subtitle");
            hb_value_t *sub_list = hb_dict_get(sub_dict, "SubtitleList");
            if (hb_value_array_len(sub_list) >= subdefault)
            {
                hb_value_t *sub = hb_value_array_get(sub_list, subdefault - 1);
                hb_dict_set(sub, "Default", hb_value_bool(1));
            }
        }

        if (subforce != NULL)
        {
            // "Forced" flag is not set during preset initialization except
            // for "foreign audio" subtitles.  Set additional request forced
            // subtitle tracks here.
            hb_value_t *sub_dict = hb_dict_get(job_dict, "Subtitle");
            hb_value_t *sub_list = hb_dict_get(sub_dict, "SubtitleList");

            int ii;
            for (ii = 0; subforce[ii] != NULL; ii++ )
            {
                int idx = strtol(subforce[ii], NULL, 0) - 1;
                if (idx >= 0 && hb_value_array_len(sub_list) > idx)
                {
                    hb_value_t *sub = hb_value_array_get(sub_list, idx);
                    hb_dict_set(sub, "Forced", hb_value_bool(1));
                }
            }
        }
    }

    if (srtfile != NULL)
    {
        int ii;
        for (ii = 0; srtfile[ii] != NULL; ii++)
        {
            add_srt(subtitle_array, ii, &one_burned);
        }
    }

    return job_dict;
}


static void print_string_list(FILE *out, const char* const *list, const char *prefix)
{
    if (out != NULL && prefix != NULL)
    {
        if (list != NULL)
        {
            while (*list != NULL)
            {
                fprintf(out, "%s%s\n", prefix, *list++);
            }
        }
        else
        {
            fprintf(out, "%s" "Option not supported by encoder\n", prefix);
        }
    }
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
static int is_whole_media_service( io_service_t service )
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
