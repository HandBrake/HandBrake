/* compat.h

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef HANDBRAKE_COMPAT_H
#define HANDBRAKE_COMPAT_H

#ifdef HB_NEED_STRTOK_R
/*
 * Some MinGW-w64 distributions #define strtok_r in pthread.h,
 * however their so-called "implementation" isn't thread-safe.
 */
#include <pthread.h>
#ifdef strtok_r
#undef strtok_r
#endif // strtok_r

char *strtok_r(char *s, const char *delim, char **save_ptr);
#endif // HB_NEED_STRTOK_R

#ifndef HAS_STRERROR_R
#ifndef _GNU_SOURCE
#include <sys/types.h>
/*
 * POSIX definition of strerror_r() -- see http://pubs.opengroup.org/onlinepubs/9699919799/functions/strerror.html
 */
int strerror_r(int errnum, char *strerrbuf, size_t buflen);
#endif // _GNU_SOURCE
#endif // HAVE_STRERROR_R

#endif // HANDBRAKE_COMPAT_H
