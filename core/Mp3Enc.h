/* $Id: Mp3Enc.h,v 1.1 2003/11/03 12:08:01 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_MP3_ENCODER_H
#define HB_MP3_ENCODER_H

#include "HandBrakeInternal.h"

HBMp3Enc * HBMp3EncInit( HBHandle *, HBAudio * );
void       HBMp3EncClose( HBMp3Enc ** );

#endif
