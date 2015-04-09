/*  HBExceptionAlertController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */
 
#import "HBExceptionAlertController.h"

@implementation HBExceptionAlertController

- (instancetype)init
{
    return [self initWithWindowNibName:@"ExceptionAlert"];
}

- (NSInteger)runModal
{
    return [NSApp runModalForWindow:self.window];
}

- (IBAction)btnCrashClicked:(id)sender
{
    [self.window orderOut:nil];
    [NSApp stopModalWithCode:HBExceptionAlertControllerResultCrash];
}

- (IBAction)btnContinueClicked:(id)sender
{
    [self.window orderOut:nil];
    [NSApp stopModalWithCode:HBExceptionAlertControllerResultContinue];
}

@end
