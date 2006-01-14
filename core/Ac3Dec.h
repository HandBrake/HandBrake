/* $Id: Ac3Dec.h,v 1.1 2003/11/03 12:08:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_AC3_DEC_H
#define HB_AC3_DEC_H

#include "HandBrakeInternal.h"

HBAc3Dec * HBAc3DecInit( HBHandle *, HBAudio * );
void       HBAc3DecClose( HBAc3Dec ** );

#endif
