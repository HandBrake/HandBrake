/* $Id: Work.h,v 1.1 2003/11/03 12:08:01 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_WORK_H
#define HB_WORK_H

#include "HandBrakeInternal.h"

#define HB_WORK_COMMON_MEMBERS \
    char   * name; \
    HBLock * lock; \
    int      used; \
    uint64_t time; \
    int      (*work) ( HBWork * );

void HBWorkLock( HBWork * );
void HBWorkWork( HBWork * );
void HBWorkUnlock( HBWork * );

HBWorkThread * HBWorkThreadInit( HBHandle *, HBTitle *, HBAudio *,
                                 HBAudio *, int firstThread );
void           HBWorkThreadClose( HBWorkThread ** );

#endif
