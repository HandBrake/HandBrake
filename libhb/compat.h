/* compat.h

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HB_COMPAT_H
#define HB_COMPAT_H

#ifdef HB_NEED_STRTOK_R
/*
 * Some MinGW-w64 distributions #define strtok_r in pthread.h,
 * however their so-called "implementation" isn't thread-safe.
 */
#ifdef USE_PTHREAD
#include <pthread.h>
#ifdef strtok_r
#undef strtok_r
#endif // strtok_r
#endif // USE_PTHREAD

char *strtok_r(char *s, const char *delim, char **save_ptr);
#endif // HB_NEED_STRTOK_R

#endif // HB_COMPAT_H
