/* $Id: main.mm,v 1.3 2005/11/25 15:04:35 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#import "hb.h"

void SigHandler( int signal )
{
    [NSApp terminate: NULL];
} 

/****************************************************************************
 * hb_error_handler
 * 
 * Change this to display a dialog box - and maybe move it somewhere else,
 * this is the only place I could find that looked like C :)
****************************************************************************/
extern "C" {
    void hb_error_handler( const char *errmsg )
    {
         fprintf(stderr, "GUI ERROR dialog: %s\n", errmsg );
    }
}

char * str_printf(const char *fmt, ...)
{
    /* Guess we need no more than 100 bytes. */
    int len;
    va_list ap;
    int size = 100;
    char *tmp, *str = NULL;

    str = (char*)malloc(size);
    while (1) 
    {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        len = vsnprintf(str, size, fmt, ap);
        va_end(ap);

        /* If that worked, return the string. */
        if (len > -1 && len < size) {
            return str;
        }

        /* Else try again with more space. */
        if (len > -1)    /* glibc 2.1 */
            size = len+1; /* precisely what is needed */
        else           /* glibc 2.0 */
            size *= 2;  /* twice the old size */

        tmp = (char*)realloc(str, size);
        if (tmp == NULL) {
            return str;
        }
        str = tmp;
    }
}

int main(int argc, const char **argv)
{
    signal(SIGINT, SigHandler);
    hb_global_init();
    hb_register_error_handler(&hb_error_handler);
    return NSApplicationMain(argc, argv);
}
