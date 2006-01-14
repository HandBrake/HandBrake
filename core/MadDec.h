/* $Id: MadDec.h,v 1.1 2003/11/03 12:08:01 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MAD_DEC_H
#define HB_MAD_DEC_H

#include "HandBrakeInternal.h"

HBMadDec * HBMadDecInit( HBHandle *, HBAudio * );
void       HBMadDecClose( HBMadDec * );

#endif
