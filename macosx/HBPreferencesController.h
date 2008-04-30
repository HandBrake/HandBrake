/**
 * @file
 * Interface of class HBPreferencesController.
 */

#import <Cocoa/Cocoa.h>

@interface HBPreferencesController : NSWindowController
{
    IBOutlet NSView         * fGeneralView, * fPictureView, * fAudioView, * fAdvancedView;
}

+ (void)registerUserDefaults;
- (id)init;

@end
