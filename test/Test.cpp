/* $Id: Test.cpp,v 1.5 2003/10/05 14:28:40 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <signal.h>

#include "Manager.h"

volatile bool die;

void SigHandler( int signal )
{
    die = true;
}

int main( int argc, char ** argv )
{
    die = false;

    /* Exit ASAP on Ctrl-C */
    signal( SIGINT,  SigHandler );

    /* Default values */
    bool   debug       = false;
    char * device      = NULL;
    char * outputFile  = NULL;
    int    titleIdx    = 1;
    bool   twoPass     = false;
    bool   deinterlace = false;
    int    width       = 0;
    int    topCrop     = 0;
    int    bottomCrop  = 0;
    int    leftCrop    = 0;
    int    rightCrop   = 0;

    /* Parse command line */
    int c;
    while( ( c = getopt( argc, argv, "vd:o:t:piw:j:k:l:m:" ) ) != -1 )
    {
        switch( c )
        {
            case 'v':
                debug = true;
                break;

            case 'd':
                device = strdup( optarg );
                break;

            case 'o':
                outputFile = strdup( optarg );
                break;

            case 't':
                titleIdx = atoi( optarg );
                break;

            case 'p':
                twoPass = true;
                break;

            case 'i':
                deinterlace = true;
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

            default:
                break;
        }
    }

    /* Check parsed options */
    if( !device || !outputFile )
    {
        fprintf( stderr,
                 "Syntax: HBTest [options] -d <device> -o <file>\n"
                 "Possible options are :\n"
                 "    -v                   verbose output\n"
                 "    -t <value>           select a title (default is 1)\n"
                 "    -p                   2-pass encoding\n"
                 "    -i                   deinterlace picture\n"
                 "    -w                   output width\n"
                 "    -j <value>           top cropping\n"
                 "    -k <value>           bottom cropping\n"
                 "    -l <value>           left cropping\n"
                 "    -m <value>           right cropping\n" );
        return 1;
    }

    /* Create the manager thread */
    HBManager * manager = new HBManager( debug );

    /* Tell the manager to scan the specified volume */
    manager->ScanVolumes( device );

    HBStatus status;
    while( !die )
    {
        if( !manager->NeedUpdate() )
        {
            Snooze( 10000 );
            continue;
        }

        status = manager->GetStatus();

        switch( status.fMode )
        {
            case HB_MODE_UNDEF:
                break;

            case HB_MODE_SCANNING:
                if( !status.fScannedTitle )
                {
                    fprintf( stderr, "Scanning %s\n",
                             status.fScannedVolume );
                }
                else
                {
                    fprintf( stderr, "Scanning %s, title %d\n",
                             status.fScannedVolume,
                             status.fScannedTitle );
                }
                break;

            case HB_MODE_READY_TO_RIP:
            {
                /* Find the title */
                HBTitle * title = NULL;
                for( uint32_t i = 0; i < status.fTitleList->CountItems(); i++ )
                {
                    title = (HBTitle*) status.fTitleList->ItemAt( i );
                    if( title->fIndex == titleIdx )
                    {
                        break;
                    }
                    else
                    {
                        title = NULL;
                    }
                }

                if( !title )
                {
                    fprintf( stderr, "Error: unvalid title. Possible "
                             "choices are: " );
                    for( uint32_t i = 0;
                         i < status.fTitleList->CountItems(); i++ )
                    {
                        title = (HBTitle*) status.fTitleList->ItemAt( i );
                        fprintf( stderr, "%d%s", title->fIndex,
                                 ( i == status.fTitleList->CountItems() - 1 )
                                 ? ".\n" : ", " );
                    }
                    die = true;
                    break;
                }
                title->fTwoPass = twoPass;
                title->fDeinterlace = deinterlace;
                if( width ) title->fOutWidth = width;
                title->fTopCrop = topCrop;
                title->fBottomCrop = bottomCrop;
                title->fLeftCrop = leftCrop;
                title->fRightCrop = rightCrop;

                HBAudio * audio = (HBAudio*) title->fAudioList->ItemAt( 0 );

                manager->StartRip( title, audio, NULL, outputFile );
                break;
            }

            case HB_MODE_ENCODING:
                fprintf( stderr, "Progress = %.2f %%, %.2f fps "
                         "(%02d:%02d:%02d remaining)\n",
                         100 * status.fPosition, status.fFrameRate,
                         status.fRemainingTime / 3600,
                         ( status.fRemainingTime % 3600 ) / 60,
                         status.fRemainingTime % 60 );
                break;

            case HB_MODE_DONE:
                fprintf( stderr, "Done\n" );
                die = true;
                break;

            case HB_MODE_CANCELED:
                fprintf( stderr, "Canceled\n" );
                die = true;
                break;

            case HB_MODE_ERROR:
                fprintf( stderr, "Error\n" );
                die = true;
                break;

            default:
                break;
        }
    }

    delete manager;

    if( device )     free( device );
    if( outputFile ) free( outputFile );

    return 0;
}

