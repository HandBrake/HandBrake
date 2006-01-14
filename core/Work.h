/* $Id: Work.h,v 1.4 2003/12/26 20:03:27 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_WORK_H
#define HB_WORK_H

#define HB_WORK_COMMON_MEMBERS \
    char   * name; \
    HBLock * lock; \
    int      used; \
    uint64_t time; \
    int      (*work) ( HBWork * );

HBWorkThread * HBWorkThreadInit( HBHandle *, HBTitle *,
                                 int firstThread );
void           HBWorkThreadClose( HBWorkThread ** );

#endif
