/* $Id: HandBrakeInternal.h,v 1.2 2003/11/04 15:44:24 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_HANDBRAKE_INTERNAL_H
#define HB_HANDBRAKE_INTERNAL_H

#include "HandBrake.h"

/* Called by HBScan to tell the GUI how far we are */
void HBScanning( HBHandle *, int title );

/* Called by HBScan. titleList is a list of all valid titles which
   should be shown on the interface */
void HBScanDone( HBHandle *, HBList * titleList );

/* Used to create temporary files (/tmp/HB.pid.whatever) */
int  HBGetPid( HBHandle * );

/* Called by every thread involved in the rip process. Returns
   immediately is rip isn't paused, blocks if it is */
void HBCheckPaused( HBHandle * );

/* Called by the decoders when the last packet is being proceeded */
void HBDone( HBHandle * );

/* Called by the video encoder to update the GUI progress */
void HBPosition( HBHandle *, float );

/* Called by any thread which couldn't continue and ask to stop */
void HBErrorOccured( HBHandle *, HBError );

#endif
