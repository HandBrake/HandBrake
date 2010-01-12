/**
 * @file
 * Interface of class HBPreferencesController.
 */

#import <Cocoa/Cocoa.h>

@interface HBPreferencesController : NSWindowController
{
    IBOutlet NSView         * fGeneralView, * fPictureView, * fAudioView, * fAdvancedView;
    IBOutlet NSTextField    * fSendEncodeToAppField;
}

+ (void)registerUserDefaults;
- (id)init;
/* Manage the send encode to xxx.app windows and field */
- (IBAction) browseSendToApp: (id) sender;
- (void) browseSendToAppDone: (NSOpenPanel *) sheet
                  returnCode: (int) returnCode contextInfo: (void *) contextInfo;
@end
