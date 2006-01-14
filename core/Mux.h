/* $Id: Mux.h,v 1.2 2004/05/02 16:25:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MUX_H
#define HB_MUX_H

#define HB_MUX_COMMON_MEMBERS \
    int (*start)    ( HBMux * ); \
    int (*muxVideo) ( HBMux *, void *, HBBuffer * ); \
    int (*muxAudio) ( HBMux *, void *, HBBuffer * ); \
    int (*end)      ( HBMux * );

typedef struct HBMux HBMux;

HBMuxThread * HBMuxThreadInit( HBHandle *, HBTitle * );
void          HBMuxThreadClose( HBMuxThread ** );

#endif
