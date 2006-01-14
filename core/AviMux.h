/* $Id: AviMux.h,v 1.1 2003/11/03 12:08:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_AVI_MUX_H
#define HB_AVI_MUX_H

#include "HandBrakeInternal.h"

HBAviMux * HBAviMuxInit( HBHandle *, HBTitle *, HBAudio *, HBAudio * );
void       HBAviMuxClose( HBAviMux ** );

#endif
