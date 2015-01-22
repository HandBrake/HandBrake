/* $Id: main.mm,v 1.3 2005/11/25 15:04:35 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

int main(int argc, const char **argv)
{
    // Register a signal handler using grand central dispatch.
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGINT, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(source, ^{
        [NSApp terminate:nil];
    });
    dispatch_resume(source);

    // Tell sigaction to ignore the SIGINT signal
    // because we handle it already with gcd.
    struct sigaction action = { {0}, 0, 0 };
    action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &action, NULL);

    return NSApplicationMain(argc, argv);
}
