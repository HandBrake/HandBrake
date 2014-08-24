/**
 * @file
 * Interface of class HBPreferencesController.
 */

#import <Cocoa/Cocoa.h>

@interface HBPreferencesController : NSWindowController <NSToolbarDelegate>

+ (void)registerUserDefaults;

@end
