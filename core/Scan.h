/* $Id: Scan.h,v 1.2 2003/11/06 13:03:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_SCAN_H
#define HB_SCAN_H

#include "HandBrakeInternal.h"

HBScan * HBScanInit( HBHandle *, char * device, int title );
void     HBScanClose( HBScan ** );

#endif
