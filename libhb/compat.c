/* compat.c

   Copyright (c) 2003-2025 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/compat.h"

#ifdef HB_NEED_STRTOK_R
#include <string.h>

char *strtok_r(char *s, const char *delim, char **save_ptr)
{
    char *token;

    if (s == NULL) s = *save_ptr;

    /* Scan leading delimiters.  */
    s += strspn(s, delim);
    if (*s == '\0') return NULL;

    /* Find the end of the token.  */
    token = s;
    s = strpbrk(token, delim);
    if (s == NULL)
    {
        /* This token finishes the string.  */
        *save_ptr = strchr(token, '\0');
    }
    else
    {
        /* Terminate the token and make *save_ptr point past it. */
        *s = '\0';
        *save_ptr = s + 1;
    }

    return token;
}
#endif // HB_NEED_STRTOK_R

#ifndef HAS_STRERROR_R
#ifndef _GNU_SOURCE
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define ERRSTR_LEN 20

int strerror_r(int errnum, char *strerrbuf, size_t buflen)
{
    int ret = 0;
    char errstr[ERRSTR_LEN];

    if (strerrbuf == NULL || buflen == 0)
    {
        ret = ERANGE;
        goto done;
    }

    if(snprintf(errstr, ERRSTR_LEN - 1, "unknown error %d", errnum) < 0)
    {
        ret = EINVAL;
        goto done;
    }

    if (snprintf(strerrbuf, buflen, errstr) < 0)
    {
        ret = EINVAL;
        goto done;
    }

done:
    return ret;
}
#endif // _GNU_SOURCE
#endif // HAS_STRERROR_R
