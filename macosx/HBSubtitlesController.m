/* $Id: HBSubtitles.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesController.h"
#import "HBSubtitlesDefaultsController.h"

#import "HBSubtitles.h"
#import "HBSubtitlesDefaults.h"

@interface HBSubtitlesController ()

// Defaults
@property (nonatomic, readwrite, strong) HBSubtitlesDefaultsController *defaultsController;

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

    [NSApp beginSheet:self.defaultsController.window
       modalForWindow:self.view.window
        modalDelegate:self
       didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:)
          contextInfo:(void *)CFBridgingRetain(defaults)];
}

- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    HBSubtitlesDefaults *defaults = (HBSubtitlesDefaults *)CFBridgingRelease(contextInfo);

    if (returnCode == NSModalResponseOK)
    {
        self.subtitles.defaults = defaults;
    }
    self.defaultsController = nil;
}

#pragma mark - Srt import

/**
 *  Imports a srt file.
 *
 *  @param sender
 */
- (IBAction)browseImportSrtFile:(id)sender
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.allowsMultipleSelection = NO;
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;

    NSURL *sourceDirectory;
    if ([[NSUserDefaults standardUserDefaults] URLForKey:@"LastSrtImportDirectoryURL"])
    {
        sourceDirectory = [[NSUserDefaults standardUserDefaults] URLForKey:@"LastSrtImportDirectoryURL"];
    }
    else
    {
        sourceDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
    }

    panel.directoryURL = sourceDirectory;
    panel.allowedFileTypes = @[@"srt"];

    [panel beginSheetModalForWindow:self.view.window completionHandler:^(NSInteger result)
    {
        if (result == NSFileHandlingPanelOKButton)
        {
            NSURL *importSrtFileURL = panel.URL;
            NSURL *importSrtDirectory = importSrtFileURL.URLByDeletingLastPathComponent;
            [[NSUserDefaults standardUserDefaults] setURL:importSrtDirectory forKey:@"LastSrtImportDirectoryURL"];

            [self.subtitles addSrtTrackFromURL:importSrtFileURL];
        }
    }];
}
@end
