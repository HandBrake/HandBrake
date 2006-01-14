#import "PrefsController.h"

@implementation PrefsController

- (void) awakeFromNib
{
    NSUserDefaults * defaults;
    NSDictionary   * appDefaults;
    
    /* Unless the user specified otherwise, default is to check
       for update */
    defaults    = [NSUserDefaults standardUserDefaults];
    appDefaults = [NSDictionary dictionaryWithObject:@"YES"
                   forKey:@"CheckForUpdates"];
    [defaults registerDefaults: appDefaults];

    /* Check or uncheck according to the preferences */
    [fUpdateCheck setState: [defaults boolForKey:@"CheckForUpdates"] ?
        NSOnState : NSOffState];
}

- (IBAction) OpenPanel: (id) sender;
{
    [NSApp runModalForWindow: fPanel];
}

- (IBAction) ClosePanel: (id) sender;
{
    [NSApp stopModal];
    [fPanel orderOut: sender];
}

- (IBAction) CheckChanged: (id) sender
{
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    
    if( [fUpdateCheck state] == NSOnState )
    {
        [defaults setObject:@"YES" forKey:@"CheckForUpdates"];
    }
    else
    {
        [defaults setObject:@"NO" forKey:@"CheckForUpdates"];
    }
}

@end
