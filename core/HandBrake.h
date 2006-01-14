/* $Id: HandBrake.h,v 1.3 2003/11/06 13:03:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_HANDBRAKE_H
#define HB_HANDBRAKE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Utils.h"

/* Init libhb. Set debug to 0 to see no output, 1 to see all libhb logs.
   Set cpuCount to 0 if you want libhb to autodetect */
HBHandle * HBInit( int debug, int cpuCount );

/* Fills the HBStatus * argument with infos about the current status.
   Returns 1 if mode has changed, 0 otherwise */
int        HBGetStatus( HBHandle *, HBStatus * );

/* Launch a thread which scans the specified device and title. Use
   title = 0 to scan all titles. Returns immediately */
void       HBScanDevice( HBHandle *, char * device, int title );

/* Start ripping the specified title with specified audio tracks.
   Returns immediatly */
void       HBStartRip( HBHandle *, HBTitle *, HBAudio *, HBAudio * );

/* Suspend rip. Returns immediatly */
void       HBPauseRip( HBHandle * );

/* Resume rip. Returns immediatly */
void       HBResumeRip( HBHandle * );

/* Cancel rip. Returns immediatly */
void       HBStopRip( HBHandle * );

/* Calculate preview for the specified picture of the specified title,
   taking care of the current cropping & scaling settings. Returns a
   pointer to raw RGBA data. It includes the white border around the
   picture, so the size of the picture is ( maxWidth + 2 ) x
   ( maxHeight + 2 ) */
uint8_t  * HBGetPreview( HBHandle *, HBTitle *, int picture );

/* Clean up things */
void       HBClose( HBHandle ** );

#ifdef __cplusplus
}
#endif

#endif
