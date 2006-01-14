/* $Id: DVDRead.h,v 1.1 2003/11/03 12:08:00 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_DVD_READ_H
#define HB_DVD_READ_H

#include "HandBrakeInternal.h"

HBDVDRead * HBDVDReadInit( HBHandle *, HBTitle *,
                           HBAudio *, HBAudio * );
void        HBDVDReadClose( HBDVDRead ** );

#endif
