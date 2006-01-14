/* $Id: HBDVDReader.cpp,v 1.7 2003/08/12 20:10:50 titer Exp $ */

#include "HBCommon.h"
#include "HBDVDReader.h"
#include "HBManager.h"
#include "HBFifo.h"

#include <Application.h>

#include <dvdread/ifo_types.h>
#include <dvdplay/dvdplay.h>
#include <dvdplay/info.h>
#include <dvdplay/state.h>
#include <dvdplay/nav.h>

HBDVDReader::HBDVDReader( HBManager * manager,
                          HBTitleInfo * titleInfo )
    : HBThread( "dvdreader", B_NORMAL_PRIORITY )
{
    fManager   = manager;
    fTitleInfo = titleInfo;
}

void HBDVDReader::DoWork()
{
    /* Open the device */
    dvdplay_ptr vmg;
    vmg = dvdplay_open( fTitleInfo->fDevice, NULL, NULL );
    if( !vmg )
    {
        Log( "HBDVDReader: dvdplay_open() failed" );
        fManager->Error();
        return;
    }
    
    /* Open the title */
    dvdplay_start( vmg, fTitleInfo->fIndex );
    
    /* Read */
    HBBuffer * dvdBuffer;
    int        beginPosition = dvdplay_position( vmg );
    int        endPosition   = dvdplay_title_end( vmg );
    while( dvdplay_position( vmg ) < endPosition )
    {
        dvdBuffer = new HBBuffer( DVD_VIDEO_LB_LEN );
        dvdBuffer->fPosition = (float) ( dvdplay_position( vmg ) - beginPosition ) /
                               (float) ( endPosition - beginPosition ) ;

        if( dvdplay_read( vmg, dvdBuffer->fData, 1 ) < 0 )
        {
            Log( "HBDVDReader: could not dvdplay_read()" );
            delete dvdBuffer;
            fManager->Error();
            break;
        }
        if( !( fTitleInfo->fPSFifo->Push( dvdBuffer ) ) )
        {
            break;
        }
    }
    
    if( dvdplay_position( vmg ) == dvdplay_title_end( vmg ) )
        fManager->Done();
    
    /* Clean up */
    dvdplay_close( vmg );
}
