/* HBAudioController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioController.h"

#import "HBAudio.h"
#import "HBAudioDefaults.h"
#import "HBAudioDefaultsController.h"

@interface HBAudioController ()

@property (nonatomic, readwrite, strong) HBAudioDefaultsController *defaultsController;

@end

@implementation HBAudioController

- (instancetype)init
{
    self = [super initWithNibName:@"Audio" bundle:nil];
    return self;
}

- (BOOL)validateUserInterfaceItem:(id < NSValidatedUserInterfaceItem >)anItem
{
    return (self.audio != nil);
}

- (IBAction)addAllAudioTracks:(id)sender
{
    [self.audio addAllTracks];
}

- (IBAction)removeAll:(id)sender
{
    [self.audio removeAll];
}

#pragma mark - Defaults

- (IBAction)showSettingsSheet:(id)sender
{
    HBAudioDefaults *defaults = [self.audio.defaults copy];
    self.defaultsController = [[HBAudioDefaultsController alloc] initWithSettings:defaults];

	[NSApp beginSheet:self.defaultsController.window
       modalForWindow:self.view.window
        modalDelegate:self
       didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:)
          contextInfo:(void *)CFBridgingRetain(defaults)];
}

- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    HBAudioDefaults *defaults = (HBAudioDefaults *)CFBridgingRelease(contextInfo);

    if (returnCode == NSModalResponseOK)
    {
        self.audio.defaults = defaults;
    }
    self.defaultsController = nil;
}

- (IBAction)reloadDefaults:(id)sender
{
    [self.audio reloadDefaults];
}

@end
