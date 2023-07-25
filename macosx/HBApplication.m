/*  HBApplication.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBApplication.h"
#import "HBExceptionAlertController.h"

@import HandBrakeKit;

@implementation HBApplication

static void CrashMyApplication(void)
{
    *(char *)0x08 = 1;
}

- (NSAttributedString *)_formattedExceptionBacktrace:(NSArray *)backtrace
{
    NSMutableAttributedString *result = [[NSMutableAttributedString alloc] init];
    for (__strong NSString *s in backtrace)
    {
        s = [s stringByAppendingString:@"\n"];
        NSAttributedString *attrS = [[NSAttributedString alloc] initWithString:s];
        [result appendAttributedString:attrS];
    }
    [result addAttribute:NSFontAttributeName value:[NSFont fontWithName:@"Monaco" size:10] range:NSMakeRange(0, result.length)];
    return result;
}

- (void)reportException:(NSException *)exception
{
    // NSApplication simply logs the exception to the console. We want to let the user know
    // when it happens in order to possibly prevent subsequent random crashes that are difficult to debug
    @try
    {
        @autoreleasepool
        {
            // Create a string based on the exception
            NSString *exceptionMessage = [NSString stringWithFormat:@"%@\nReason: %@\nUser Info: %@", exception.name, exception.reason, exception.userInfo];
            // Always log to console for history

            [HBUtilities writeToActivityLog:"Exception raised:\n%s", exceptionMessage.UTF8String];
            [HBUtilities writeToActivityLog:"Backtrace:\n%s", exception.callStackSymbols.description.UTF8String];

            HBExceptionAlertController *alertController = [[HBExceptionAlertController alloc] init];
            alertController.exceptionMessage = exceptionMessage;
            alertController.exceptionBacktrace = [self _formattedExceptionBacktrace:exception.callStackSymbols];

            NSInteger result = [alertController runModal];
            if (result == HBExceptionAlertControllerResultCrash)
            {
                CrashMyApplication();
            }
        }
    }
    @catch (NSException *e)
    {
        // Suppress any exceptions raised in the handling
    }
}

@end
