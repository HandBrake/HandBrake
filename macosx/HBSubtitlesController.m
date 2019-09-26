/* $Id: HBSubtitles.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesController.h"
#import "HBSubtitlesDefaultsController.h"
#import "HBTrackTitleViewController.h"

@import HandBrakeKit;

@interface HBSubtitlesController ()

@property (nonatomic, readwrite, strong) HBSubtitlesDefaultsController *defaultsController;
@property (nonatomic, weak) IBOutlet NSTableView *table;

@end

@implementation HBSubtitlesController

- (instancetype)init
{
    self = [super initWithNibName:@"Subtitles" bundle:nil];
    return self;
}

#pragma mark - Actions

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)anItem
{
    return (self.subtitles != nil);
}

/**
 *  Add every subtitles track that still isn't in the subtitles array.
 */
- (IBAction)addAll:(id)sender
{
    [self.subtitles addAllTracks];
}

/**
 *  Remove all the subtitles tracks.
 */
- (IBAction)removeAll:(id)sender
{
    [self.subtitles removeAll];
}

/**
 *  Remove all the subtitles tracks and
 *  add new ones based on the defaults settings
 */
- (IBAction)addTracksFromDefaults:(id)sender
{
    [self.subtitles reloadDefaults];
}

- (IBAction)showSettingsSheet:(id)sender
{
    HBSubtitlesDefaults *defaults = [self.subtitles.defaults copy];
    self.defaultsController = [[HBSubtitlesDefaultsController alloc] initWithSettings:defaults];

    [self.view.window beginSheet:self.defaultsController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK)
        {
            self.subtitles.defaults = defaults;
        }
        self.defaultsController = nil;
    }];
}

- (IBAction)showAdditionalSettingsPopOver:(id)sender
{
    HBTrackTitleViewController *controller = [[HBTrackTitleViewController alloc] init];
    NSInteger index = [self.table rowForView:sender];
    if (index != -1)
    {
        controller.track = [self.subtitles objectInTracksAtIndex:index];
        [self presentViewController:controller asPopoverRelativeToRect:[sender bounds] ofView:sender preferredEdge:NSRectEdgeMinX behavior:NSPopoverBehaviorTransient];
    }
}

#pragma mark - External subtitles import

/**
 *  Imports a srt/ssa file.
 *
 *  @param sender
 */
- (IBAction)browseImportExternalFile:(id)sender
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.allowsMultipleSelection = NO;
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;

    NSURL *sourceDirectory;
    if ([[NSUserDefaults standardUserDefaults] URLForKey:@"LastExternalSubImportDirectoryURL"])
    {
        sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"LastExternalSubImportDirectoryURL"];
    }
    else
    {
        sourceDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop" isDirectory:YES];
    }

    panel.directoryURL = sourceDirectory;
    panel.allowedFileTypes = @[@"srt", @"ssa", @"ass"];

    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSInteger result)
    {
        if (result == NSModalResponseOK)
        {
            NSURL *importFileURL = panel.URL;
            NSURL *importDirectory = importFileURL.URLByDeletingLastPathComponent;
            [[NSUserDefaults standardUserDefaults] setURL:importDirectory forKey:@"LastExternalSubImportDirectoryURL"];

            [self.subtitles addExternalTrackFromURL:importFileURL];
        }
    }];
}
@end
