/* HBAudioController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioController.h"

#import "HBAudio.h"
#import "HBAudioDefaultsController.h"

@interface HBAudioController ()

@property (nonatomic, readwrite, retain) HBAudioDefaultsController *defaultsController;

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
    self.defaultsController = [[[HBAudioDefaultsController alloc] initWithSettings:self.audio.defaults] autorelease];

	[NSApp beginSheet:[self.defaultsController window]
       modalForWindow:[[self view] window]
        modalDelegate:self
       didEndSelector:@selector(sheetDidEnd)
          contextInfo:NULL];
}

- (void)sheetDidEnd
{
    self.defaultsController = nil;
}

- (IBAction)reloadDefaults:(id)sender
{
    [self.audio reloadDefaults];
}

@end
