/* $Id: test.c,v 1.7 2003/11/13 01:18:52 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <signal.h>

#include "HandBrake.h"

volatile int die;

void SigHandler( int signal )
{
    die = 1;
}

int main( int argc, char ** argv )
{
    int c;
    HBHandle * h;
    HBStatus   s;

    /* Default values */
    int    debug       = 1;
    char * device      = NULL;
    char * file        = NULL;
    int    titleIdx    = 1;
    int    audio1Idx   = 1;
    int    audio2Idx   = 0;
    int    twoPass     = 0;
    int    deinterlace = 0;
    int    width       = 0;
    int    topCrop     = 0;
    int    bottomCrop  = 0;
    int    leftCrop    = 0;
    int    rightCrop   = 0;
    int    cpuCount    = 0;
    int    vBitrate    = 1024;
    int    aBitrate    = 128;
    int    xvid        = 0;
   
    die = 0;

    /* Exit ASAP on Ctrl-C */
    signal( SIGINT,  SigHandler );

    fprintf( stderr, "Welcome to HandBrake " VERSION "\n" );

    /* Parse command line */
    while( ( c = getopt( argc, argv,
                         "qd:o:t:a:b:piw:j:k:l:m:c:e:f:x" ) ) != -1 )
    {
        switch( c )
        {
            case 'q':
                debug = 0;
                break;

            case 'd':
                device = strdup( optarg );
                break;

            case 'o':
                file = strdup( optarg );
                break;

            case 't':
                titleIdx = atoi( optarg );
                break;

            case 'a':
                audio1Idx = atoi( optarg );
                break;

            case 'b':
                audio2Idx = atoi( optarg );
                break;

            case 'p':
                twoPass = 1;
                break;

            case 'i':
                deinterlace = 1;
                break;

            case 'w':
                width = atoi( optarg );
                break;

            case 'j':
                topCrop = atoi( optarg );
                break;

            case 'k':
                bottomCrop = atoi( optarg );
                break;

            case 'l':
                leftCrop = atoi( optarg );
                break;

            case 'm':
                rightCrop = atoi( optarg );
                break;

            case 'c':
                cpuCount = atoi( optarg );
                break;

            case 'e':
                vBitrate = atoi( optarg );
                break;

            case 'f':
                aBitrate = atoi( optarg );
                break;

            case 'x':
                xvid = 1;
                break;

            default:
                break;
        }
    }

    /* Check parsed options */
    if( !device || !file )
    {
        fprintf( stderr,
            "Syntax: HBTest [options] -d <device> -o <file>\n"
            "Possible options are :\n"
            "    -q           quiet output\n"
            "    -t <value>   select a title (default is 1)\n"
            "    -a <value>   primary audio channel (default is 1)\n"
            "    -b <value>   secondary audio channel (default is none)\n"
            "    -p           2-pass encoding\n"
            "    -i           deinterlace picture\n"
            "    -w           output width\n"
            "    -j <value>   top cropping\n"
            "    -k <value>   bottom cropping\n"
            "    -l <value>   left cropping\n"
            "    -m <value>   right cropping\n"
            "    -c <value>   CPU count (default: autodetected)\n"
            "    -e <value>   Video bitrate (default is 1024)\n"
            "    -f <value>   Audio bitrate (default is 128)\n"
            "    -x           Use XviD instead of Ffmpeg\n" );
        return 1;
    }

    /* Create the lihb thread & init things */
    h = HBInit( debug, cpuCount );

    while( !die )
    {
        HBSnooze( 100000 );

        if( !HBGetStatus( h, &s ) )
            continue;

        switch( s.mode )
        {
            case HB_MODE_UNDEF:
                /* Will never happen */
                break;

            case HB_MODE_NEED_DEVICE:
                /* Feed libhb with a DVD to scan */
                HBScanDevice( h, device, titleIdx );
                break;

            case HB_MODE_SCANNING:
                /* s.scannedTitle: title scanned at the moment */
                break;

            case HB_MODE_INVALID_DEVICE:
                die = 1;
                break;

            case HB_MODE_READY_TO_RIP:
            {
                HBAudio * audio1, * audio2;
                HBTitle * title = HBListItemAt( s.titleList, 0 );

                title->file = strdup( file );
                title->twoPass = twoPass;
                title->deinterlace = deinterlace;
                if( width ) title->outWidth = width;
                title->topCrop = topCrop;
                title->bottomCrop = bottomCrop;
                title->leftCrop = leftCrop;
                title->rightCrop = rightCrop;
                title->bitrate = vBitrate;
                title->codec = xvid ? HB_CODEC_XVID : HB_CODEC_FFMPEG;

                audio1 = HBListItemAt( title->audioList,
                                       audio1Idx - 1 );
                audio2 = HBListItemAt( title->audioList,
                                       audio2Idx - 1 );
                if( audio1 ) audio1->outBitrate = aBitrate;
                if( audio2 ) audio2->outBitrate = aBitrate;

                HBStartRip( h, title, audio1, audio2 );
                break;
            }

            case HB_MODE_ENCODING:
                /* s.position     : current progress (0.0->1.0)
                   s.frameRate    : average framerate
                   s.remainingTime: ... (in seconds) */
                break;

            case HB_MODE_DONE:
                die = 1;
                break;

            case HB_MODE_CANCELED:
                die = 1;
                break;

            case HB_MODE_ERROR:
                /* s.error: error code */
                die = 1;
                break;

            default:
                break;
        }
    }

    HBClose( &h );

    if( device ) free( device );
    if( file )   free( file );

    return 0;
}

