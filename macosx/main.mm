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

#define EXTRA_VLC_DYLD_PATH "/Applications/VLC.app/Contents/MacOS/lib"
#define DEFAULT_DYLD_PATH "/usr/local/lib:/usr/lib"

int main( int argc, const char ** argv )
{
    char *dylib_path;
    int no_exec = 0;

    // Check for flag that prevents exec bomb.  It
    // incidentally can be used to prevent adding
    // our modifications to the dyld env vars.
    if ( argc > 1 && strncmp(argv[1], "-n", 2) == 0 )
        no_exec = 1;

    if ( !no_exec )
    {
        dylib_path = getenv("DYLD_FALLBACK_LIBRARY_PATH");
        if ( dylib_path == NULL ||
             strstr( dylib_path, "/Applications/VLC.app/Contents/MacOS/lib" ) == NULL )
        {
            char *path = NULL;
            char *home;
            int result = -1;

            home = getenv("HOME");

            if ( dylib_path == NULL )
            {
                // Set the system default of $HOME/lib:/usr/local/lib:/usr/lib
                // And add our extra path
                if ( home != NULL )
                {
                    path = str_printf("%s/lib:%s:%s:%s%s", home, 
                                      DEFAULT_DYLD_PATH, 
                                      EXTRA_VLC_DYLD_PATH, 
                                      home, EXTRA_VLC_DYLD_PATH);
                }
                else
                {
                    path = str_printf("%s:%s", DEFAULT_DYLD_PATH, EXTRA_VLC_DYLD_PATH);
                }
                if ( path != NULL )
                    result = setenv("DYLD_FALLBACK_LIBRARY_PATH", path, 1);
            }
            else
            {
                // add our extra path
                if ( home != NULL )
                {
                    path = str_printf("%s:%s:%s%s", dylib_path, EXTRA_VLC_DYLD_PATH,
                                                        home, EXTRA_VLC_DYLD_PATH);
                }
                else
                {
                    path = str_printf("%s:%s", dylib_path, EXTRA_VLC_DYLD_PATH);
                }
                if ( path != NULL )
                    result = setenv("DYLD_FALLBACK_LIBRARY_PATH", path, 1);
            }
            if ( result == 0 )
            {
                const char ** new_argv;
                int i;

                new_argv = (const char**)malloc( (argc + 2) * sizeof(char*) );
                new_argv[0] = argv[0];
                new_argv[1] = "-n";
                for (i = 1; i < argc; i++)
                    new_argv[i+1] = argv[i];
                new_argv[i+1] = NULL;
                execv(new_argv[0], (char* const*)new_argv);
            }
        }
    }
    signal( SIGINT, SigHandler );
    hb_register_error_handler(&hb_error_handler);
    return NSApplicationMain( argc, argv );
}
