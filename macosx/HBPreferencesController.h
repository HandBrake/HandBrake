/**
 * @file
 * Interface of class HBPreferencesController.
 */

#import <Cocoa/Cocoa.h>

typedef NS_ENUM(NSUInteger, HBDoneAction) {
    HBDoneActionAlert = 1,
    HBDoneActionNotification = 2,
    HBDoneActionAlertAndNotification = 3,
    HBDoneActionSleep = 4,
    HBDoneActionShutDown = 5,
};

@interface HBPreferencesController : NSWindowController <NSToolbarDelegate>

+ (void)registerUserDefaults;

@end
