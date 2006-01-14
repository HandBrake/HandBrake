/* $Id: HandBrake.h,v 1.10 2004/01/21 18:40:36 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_HANDBRAKE_H
#define HB_HANDBRAKE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Utils.h"

/* Interface callbacks */
typedef struct HBCallbacks
{
    void * data;

    void (*scanning) ( void * data, int title, int titleCount );
    void (*scanDone) ( void * data, HBList * titleList );
    void (*encoding) ( void * data, float position, int pass,
                       int passCount, float curFrameRate,
                       float avgFrameRate, int remainingTime );
    void (*ripDone)  ( void * data, int result );

} HBCallbacks;

/* Init libhb. Set debug to 0 to see no output, 1 to see all libhb logs.
   Set cpuCount to 0 if you want libhb to autodetect */
HBHandle * HBInit( int debug, int cpuCount );

/* Tell libhb what functions should be called when a GUI should be
   updated. */
void       HBSetCallbacks( HBHandle *, HBCallbacks callbacks );

/* Launch a thread which scans the specified DVD and title. Use
   title = 0 to scan all titles. Returns immediately */
void       HBScanDVD( HBHandle *, const char * dvd, int title );

/* Calculate bitrate so the output file fits in X MB */
int        HBGetBitrateForSize( HBTitle * title, int size, int muxer,
                                int audioCount, int audioBitrate );

/* Start ripping the specified title. Returns immediatly */
void       HBStartRip( HBHandle *, HBTitle * );

/* Suspend rip */
void       HBPauseRip( HBHandle * );

/* Resume rip */
void       HBResumeRip( HBHandle * );

/* Cancel rip. Returns immediatly - you'll be noticed by the ripDone
   callback when it's really stopped.
   If the rip was paused, you _must_ call HBResumeRip() first. */
void       HBStopRip( HBHandle * );

/* Calculate preview for the specified picture of the specified title,
   taking care of the current cropping & scaling settings. Returns a
   pointer to raw RGBA data that _has_ to be freed by the calling
   function. The picture includes the white border around the picture,
   so its size is ( maxWidth + 2 ) x ( maxHeight + 2 ).
   The data belongs to the caller, who must free it. */
uint8_t  * HBGetPreview( HBHandle *, HBTitle *, int picture );

/* Clean up things */
void       HBClose( HBHandle ** );

#ifdef __cplusplus
}
#endif

#endif
