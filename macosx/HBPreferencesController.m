/**
 * @file
 * Implementation of class HBPreferencesController.
 */

#import "HBPreferencesController.h"

/**
 * This class controls the preferences window of HandBrake. Default values for
 * all preferences and user defaults are specified in class method
 * @c registerUserDefaults. The preferences window is loaded from
 * Preferences.nib file when HBPreferencesController is initialized.
 *
 * All preferences are bound to user defaults in Interface Builder, therefore
 * no getter/setter code is needed in this file (unless more complicated
 * preference settings are added that cannot be handled with Cocoa bindings).
 */
@implementation HBPreferencesController

/**
 * Registers default values to user defaults. This is called immediately
 * when HandBrake starts, from [HBController init].
 */
+ (void)registerUserDefaults
{
    NSString *desktopDirectory =  [@"~/Desktop" stringByExpandingTildeInPath];
    
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
        @"YES",             @"CheckForUpdates",
        @"English",         @"DefaultLanguage",
        @"NO",              @"DefaultMpegName",
        @"YES",             @"DefaultCrf",
        @"NO",              @"DefaultDeinterlaceOn",
        @"YES",             @"DefaultPicSizeAutoiPod",
        @"NO",              @"PixelRatio",
        @"",                @"DefAdvancedx264Flags",
        @"YES",             @"DefaultPresetsDrawerShow",
        desktopDirectory,   @"LastDestinationDirectory",
        desktopDirectory,   @"LastSourceDirectory",
        @"NO",              @"DefaultAutoNaming",
        @"NO",              @"DefaultChapterMarkers",
        @"NO",              @"ShowVerboseOutput",
		@"NO",              @"AllowLargeFiles",
		@"NO",              @"DisableDvdAutoDetect",
        nil]];
}

/**
 * Initializes the preferences controller by loading Preferences.nib file.
 */
- (id)init
{
    if (self = [super initWithWindowNibName:@"Preferences"])
    {
        NSAssert([self window], @"[HBPreferencesController init] window outlet is not connected in Preferences.nib");
    }
    return self; 
}

/**
 * Shows the preferences window in modal state.
 */
- (IBAction)runModal:(id)sender
{
    [NSApp runModalForWindow:[self window]];
}

/**
 * Closes the window and stops modal state. Any changes made in field editor
 * are saved by [NSWindow endEditingFor:] before closing the window.
 */
- (IBAction)close:(id)sender
{
    [[self window] endEditingFor:nil];
    [[self window] orderOut:sender];
    [NSApp stopModal];
}

@end
