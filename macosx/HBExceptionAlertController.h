/*  HBExceptionAlertController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

typedef NS_ENUM(NSUInteger, HBExceptionAlertControllerResult) {
    HBExceptionAlertControllerResultCrash,
    HBExceptionAlertControllerResultContinue,
};

@interface HBExceptionAlertController : NSWindowController

// Properties are used by bindings
@property (copy) NSString *exceptionMessage;
@property (copy) NSAttributedString *exceptionBacktrace;

- (IBAction)btnCrashClicked:(id)sender;
- (IBAction)btnContinueClicked:(id)sender;

- (NSInteger)runModal;

@end
