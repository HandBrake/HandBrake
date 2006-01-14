/* $Id: DVDReader.cpp,v 1.12 2003/10/03 16:49:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include "DVDReader.h"
#include "Manager.h"
#include "Fifo.h"

#include <dvdread/ifo_types.h>
#include <dvdplay/dvdplay.h>
#include <dvdplay/info.h>
#include <dvdplay/state.h>
#include <dvdplay/nav.h>

HBDVDReader::HBDVDReader( HBManager * manager, HBTitle * title )
    : HBThread( "dvdreader" )
{
    fManager = manager;
    fTitle   = title;
}

void HBDVDReader::DoWork()
{
    /* Open the device */
    dvdplay_ptr vmg;
    vmg = dvdplay_open( fTitle->fDevice, NULL, NULL );
    if( !vmg )
    {
        Log( "HBDVDReader: dvdplay_open() failed" );
        fManager->Error();
        return;
    }

    bool die = false;
    for( int i = 0; i < ( fTitle->fTwoPass ? 2 : 1 ); i++ )
    {
        /* Open the title */
        dvdplay_start( vmg, fTitle->fIndex );

        /* Read */
        HBBuffer * dvdBuffer;
        int        beginPosition = dvdplay_position( vmg );
        int        endPosition   = dvdplay_title_end( vmg );

        while( dvdplay_position( vmg ) < endPosition )
        {
            while( fSuspend )
            {
                Snooze( 10000 );
            }

            dvdBuffer = new HBBuffer( DVD_VIDEO_LB_LEN );
            dvdBuffer->fPosition = (float) ( dvdplay_position( vmg )
                                             - beginPosition ) /
                                   (float) ( endPosition - beginPosition ) ;
            if( fTitle->fTwoPass )
            {
                dvdBuffer->fPosition /= 2;
                if( i == 1 )
                {
                    dvdBuffer->fPosition += 0.5;
                }
            }
            dvdBuffer->fPass = fTitle->fTwoPass ? ( i + 1 ) : 0;

            if( dvdplay_read( vmg, dvdBuffer->fData, 1 ) < 0 )
            {
                Log( "HBDVDReader: could not dvdplay_read()" );
                delete dvdBuffer;
                fManager->Error();
                break;
            }

            if( !Push( fTitle->fPSFifo, dvdBuffer ) )
            {
                die = true;
                break;
            }
        }

        if( die )
        {
            break;
        }
    }

    if( !die )
    {
        fManager->Done();
    }

    /* Clean up */
    dvdplay_close( vmg );
}
