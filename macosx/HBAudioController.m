/* HBAudioController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioController.h"
#import "HBAudioDefaultsController.h"
#import "HBTrackTitleViewController.h"

@import HandBrakeKit;

@interface HBAudioController ()

@property (nonatomic, readwrite, strong) HBAudioDefaultsController *defaultsController;
@property (nonatomic, weak) IBOutlet NSTableView *table;

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

- (IBAction)showAdditionalSettingsPopOver:(id)sender
{
    HBTrackTitleViewController *controller = [[HBTrackTitleViewController alloc] init];
    NSInteger index = [self.table rowForView:sender];
    if (index != -1)
    {
        controller.track = [self.audio objectInTracksAtIndex:index];
        [self presentViewController:controller asPopoverRelativeToRect:[sender bounds] ofView:sender preferredEdge:NSRectEdgeMinX behavior:NSPopoverBehaviorSemitransient];
    }
}

#pragma mark - Defaults

- (IBAction)showSettingsSheet:(id)sender
{
    HBAudioDefaults *defaults = [self.audio.defaults copy];
    self.defaultsController = [[HBAudioDefaultsController alloc] initWithSettings:defaults];

    [self.view.window beginSheet:self.defaultsController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK)
        {
            self.audio.defaults = defaults;
        }
        self.defaultsController = nil;
    }];
}

- (IBAction)reloadDefaults:(id)sender
{
    [self.audio reloadDefaults];
}

@end
