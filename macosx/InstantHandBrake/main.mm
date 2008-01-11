/* $Id: main.mm,v 1.3 2005/11/25 15:04:35 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>
#import "hb.h"

void SigHandler( int signal )
{
    [NSApp terminate: NULL];
} 

int main( int argc, const char ** argv )
{
    signal( SIGINT, SigHandler );
    return NSApplicationMain( argc, argv );
}
