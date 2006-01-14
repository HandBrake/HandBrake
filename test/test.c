/* $Id: test.c,v 1.28 2004/03/22 19:18:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <signal.h>
#include <getopt.h>

#include "HandBrake.h"

/* Options */
static int    verbose     = 0;
static char * input       = NULL;
static char * output      = NULL;
static char * format      = NULL;
static int    titleindex  = 1;
static int    twoPass     = 0;
static int    deinterlace = 0;
static int    vcodec      = HB_CODEC_FFMPEG;
static char * audios      = NULL;
static int    width       = 0;
static int    top         = 0;
static int    bottom      = 0;
static int    left        = 0;
static int    right       = 0;
static int    autocrop    = 0;
static int    cpu         = 0;
static int    vbitrate    = 1024;
static int    size        = 0;
static int    abitrate    = 128;
static int    mux         = 0;
static int    acodec      = 0;

/* Exit cleanly on Ctrl-C */
static volatile int die = 0;
static void SigHandler( int );

/* Utils */
static void ShowHelp();
static int  ParseOptions( int argc, char ** argv );
static int  CheckOptions( int argc, char ** argv );

/* libhb callbacks */
static void Scanning( void * data, int title, int titleCount );
static void ScanDone( void * data, HBList * titleList );
static void Encoding( void * data, float position, int pass,
                      int passCount, float frameRate,
                      float avgFrameRate, int remainingTime );
static void RipDone( void * data, int result );

int main( int argc, char ** argv )
{
    HBHandle    * h;
    HBCallbacks   callbacks;

    fprintf( stderr, "HandBrake " HB_VERSION
             " - http://handbrake.m0k.org/\n" );

    if( ParseOptions( argc, argv ) ||
        CheckOptions( argc, argv ) )
    {
        return 1;
    }

    /* Exit ASAP on Ctrl-C */
    signal( SIGINT, SigHandler );

    /* Init libhb */
    h = HBInit( verbose, cpu );

    /* Set libhb callbacks */
    callbacks.data     = h;
    callbacks.scanning = Scanning;
    callbacks.scanDone = ScanDone;
    callbacks.encoding = Encoding;
    callbacks.ripDone  = RipDone;
    HBSetCallbacks( h, callbacks );

    /* Feed libhb with a DVD to scan */
    fprintf( stderr, "Opening %s...\n", input );
    HBScanDVD( h, input, titleindex );

    /* Wait... */
    while( !die )
    {
        HBSnooze( 500000 );
    }

    /* Clean up */
    HBClose( &h );

    fprintf( stderr, "HandBrake has exited cleanly.\n" );

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
        i_die_date = HBGetDate();
        fprintf( stderr, "Signal %d received, terminating - do it "
                 "again in case it gets stuck\n", i_signal );
    }
    else if( i_die_date + 500000 < HBGetDate() )
    {
        fprintf( stderr, "Dying badly, files might remain in your /tmp\n" );
        exit( 1 );
    }
    die = 1;
}

/****************************************************************************
 * ShowHelp:
 ****************************************************************************/
static void ShowHelp()
{
    fprintf( stderr,
    "Syntax: HBTest [options] -i <device> -o <file>\n"
    "\n"
    "    -h, --help            Print help\n"
    "    -v, --verbose         Be verbose\n"
    "    -C, --cpu             Set CPU count (default: autodetected)\n"
    "\n"
    "    -f, --format <string> Set output format (avi/mp4/ogm, default:\n"
    "                          autodetected)\n"
    "    -i, --input <string>  Set input device\n"
    "    -o, --output <string> Set output file name\n"
    "\n"
    "        --scan            Only scan the device\n"
    "    -t, --title <number>  Select a title to encode (0 to scan only,\n"
    "                          default: 1)\n"
    "    -a, --audio <string>  Select audio channel(s) (none for no audio,\n"
    "                          default: first one)\n"
    "        --noaudio         Disable audio\n"
    "\n"
    "    -c, --codec <string>  Set video library encoder (ffmpeg/xvid/x264,\n"
    "                          default: ffmpeg)\n"
    "    -2, --two-pass        Use two-pass mode\n"
    "    -d, --deinterlace     Deinterlace video\n"
    "\n"
    "    -b, --vb <kb/s>       Set video bitrate (default: 1024)\n"
    "    -s, --size <MB>       Set target size instead of bitrate\n"
    "    -B, --ab <kb/s>       Set audio bitrate (default: 128)\n"
    "    -w, --width <number>  Set picture width\n"
    "        --crop <T:B:L:R>  Set cropping values\n"
    "        --autocrop        Use autodetected cropping values\n" );
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
            { "verbose",     no_argument,       NULL,    'v' },
            { "cpu",         required_argument, NULL,    'C' },

            { "format",      required_argument, NULL,    'f' },
            { "input",       required_argument, NULL,    'i' },
            { "output",      required_argument, NULL,    'o' },

            { "scan",        no_argument,       NULL,    'S' },
            { "title",       required_argument, NULL,    't' },
            { "audio",       required_argument, NULL,    'a' },
            { "noaudio",     no_argument,       NULL,    'a' },

            { "codec",       required_argument, NULL,    'c' },
            { "two-pass",    no_argument,       NULL,    '2' },
            { "deinterlace", no_argument,       NULL,    'd' },
            { "width",       required_argument, NULL,    'w' },
            { "crop",        required_argument, NULL,    'n' },
            { "autocrop",    no_argument,       NULL,    'z' },

            { "vb",          required_argument, NULL,    'b' },
            { "size",        required_argument, NULL,    's' },
            { "ab",          required_argument, NULL,    'B' },

            { 0, 0, 0, 0 }
          };

        int option_index = 0;
        int c;

        c = getopt_long( argc, argv, "hvC:f:i:o:St:a:c:2dw:n:zb:s:B:",
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
            case 'v':
                verbose = 1;
                break;
            case 'C':
                cpu = atoi( optarg );
                break;

            case 'f':
                format = strdup( optarg );
                break;
            case 'i':
                input = strdup( optarg );
                break;
            case 'o':
                output = strdup( optarg );
                break;

            case 'S':
                titleindex = 0;
                break;
            case 't':
                titleindex = atoi( optarg );
                break;
            case 'a':
                audios = strdup( optarg ? optarg : "none" );
                break;

            case '2':
                twoPass = 1;
                break;
            case 'd':
                deinterlace = 1;
                break;
            case 'c':
                if( !strcasecmp( optarg, "ffmpeg" ) )
                {
                    vcodec = HB_CODEC_FFMPEG;
                }
                else if( !strcasecmp( optarg, "xvid" ) )
                {
                    vcodec = HB_CODEC_XVID;
                }
                else if( !strcasecmp( optarg, "x264" ) )
                {
                    vcodec = HB_CODEC_X264;
                }
                else
                {
                    fprintf( stderr, "invalid codec (%s)\n", optarg );
                    return -1;
                }
                break;
            case 'w':
                width = atoi( optarg );
                break;
            case 'n':
            {
                char * crop = strdup( optarg );
                char * _2be3 = crop;

                if( *crop )
                {
                    top = strtol( crop, &crop, 0 ); crop++;
                }
                if( *crop )
                {
                    bottom = strtol( crop, &crop, 0 ); crop++;
                }
                if( *crop )
                {
                    left = strtol( crop, &crop, 0 ); crop++;
                }
                if( *crop )
                {
                    right = strtol( crop, &crop, 0 ); crop++;
                }

                free( _2be3 );
                break;
            }
            case 'z':
               autocrop = 1;
               break;

            case 'b':
                vbitrate = atoi( optarg );
                break;
            case 's':
                size = atoi( optarg );
                break;
            case 'B':
                abitrate = atoi( optarg );
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
            char *p = strrchr( output, '.' );
            /* autodetect */
            if( p && !strcasecmp( p, ".avi" ) )
            {
                mux = HB_MUX_AVI;
            }
            else if( p && !strcasecmp( p, ".mp4" ) )
            {
                mux = HB_MUX_MP4;
            }
            else if( p && ( !strcasecmp( p, ".ogm" ) ||
                            !strcasecmp( p, ".ogg" ) ) )
            {
                mux = HB_MUX_OGM;
            }

            else
            {
                fprintf( stderr, "Output format couldn't be guessed "
                         "from file name, please use --format.\n" );
                return 1;
            }
        }
        else if( !strcasecmp( format, "avi" ) )
        {
            mux = HB_MUX_AVI;
        }
        else if( !strcasecmp( format, "mp4" ) )
        {
            mux = HB_MUX_MP4;
        }
        else if( !strcasecmp( format, "ogm" ) ||
                 !strcasecmp( format, "ogg" ) )
        {
            mux = HB_MUX_OGM;
        }
        else
        {
            fprintf( stderr, "Invalid output format (%s). Possible "
                     "choices are avi, mp4 and ogm\n.", format );
            return 1;
        }
        if( mux == HB_MUX_MP4 )
        {
            acodec = HB_CODEC_AAC;
        }
        else if( mux == HB_MUX_AVI )
        {
            acodec = HB_CODEC_MP3;
        }
        else if( mux == HB_MUX_OGM )
        {
            acodec = HB_CODEC_VORBIS;
        }
    }

    return 0;
}

static void Scanning( void * data, int title, int titleCount )
{
    if( titleindex )
    {
        fprintf( stderr, "Scanning title %d...\n", title );
    }
    else
    {
        fprintf( stderr, "Scanning title %d/%d...\n", title, titleCount );
    }
}

static void ScanDone( void * data, HBList * titleList )
{
    HBHandle * h = (HBHandle*) data;
    HBAudio  * audio;
    HBTitle  * title;

    if( !titleList )
    {
        fprintf( stderr, "No title found. Invalid device?\n" );
        die = 1;
        return;
    }
    if( !titleindex )
    {
        die = 1;
        return;
    }

    title = HBListItemAt( titleList, 0 );
    title->file = strdup( output );
    title->twoPass = twoPass;
    title->deinterlace = deinterlace;
    if( width )
    {
        title->outWidth = width;
    }
    if( autocrop )
    {
        title->topCrop    = title->autoTopCrop;
        title->bottomCrop = title->autoBottomCrop;
        title->leftCrop   = title->autoLeftCrop;
        title->rightCrop  = title->autoRightCrop;
    }
    else
    {
        title->topCrop    = top;
        title->bottomCrop = bottom;
        title->leftCrop   = left;
        title->rightCrop  = right;
    }
    fprintf( stderr, "Cropping: T=%d,B=%d,L=%d,R=%d\n",
             title->topCrop, title->bottomCrop,
             title->leftCrop, title->rightCrop );
    title->bitrate = vbitrate;
    title->codec = vcodec;
    title->mux = mux;

    if( audios == NULL )
    {
        audio = HBListItemAt( title->audioList, 0 );
        audio->outBitrate = abitrate;
        audio->outCodec = acodec;
        HBListAdd( title->ripAudioList, audio );
    }
    else if( strcasecmp( audios, "none" ) )
    {
        char *tmp = audios;

        while( *tmp )
        {
            int i;

            if( *tmp < '0' || *tmp > '9' )
            {
                /* Skip non numeric char */
                tmp++;
                continue;
            }

            i = strtol( tmp, &tmp, 0 );
            audio = HBListItemAt( title->audioList, i - 1 );
            audio->outBitrate = abitrate;
            audio->outCodec = acodec;
            HBListAdd( title->ripAudioList, audio );
        }
    }
    if( size )
    {
        title->bitrate = HBGetBitrateForSize( title, size, title->mux,
                HBListCount( title->ripAudioList ), abitrate );
        fprintf( stderr, "Calculated bitrate: %d kbps\n", title->bitrate );
    }

    HBStartRip( h, title );
}

static void Encoding( void * data, float position, int pass,
                      int passCount, float frameRate,
                      float avgFrameRate, int remainingTime )
{
    fprintf( stderr, "%6.2f %% (pass: %d/%d, cur/avg speed: "
             "%5.2f/%5.2f fps, %02d:%02d:%02d remaining)\n",
             100.0 * position, pass, passCount, frameRate, avgFrameRate,
             remainingTime / 3600, ( remainingTime / 60 ) % 60,
             remainingTime % 60 );
}

static void RipDone( void * data, int result )
{
    switch( result )
    {
        case HB_SUCCESS:
            fprintf( stderr, "Rip done!\n" );
            break;
        case HB_CANCELED:
            fprintf( stderr, "Rip canceled.\n" );
            break;
        default:
            fprintf( stderr, "Rip failed (error %x).\n", result );
    }
    die = 1;
}

