/* compat.c

   Copyright (c) 2003-2015 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "compat.h"

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
