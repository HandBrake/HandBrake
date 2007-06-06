/**
 * @file
 * Interface of class HBPreferencesController.
 */

#import <Cocoa/Cocoa.h>

@interface HBPreferencesController : NSWindowController
{
}

+ (void)registerUserDefaults;
- (id)init;
- (IBAction)runModal:(id)sender;
- (IBAction)close:(id)sender;

@end
