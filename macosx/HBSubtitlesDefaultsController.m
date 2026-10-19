/*  HBSubtitlesDefaultsController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesDefaultsController.h"
#import "HBLanguagesSelection.h"

@import HandBrakeKit;

static void *HBSubtitlesDefaultsContext = &HBSubtitlesDefaultsContext;

@interface HBSubtitlesDefaultsController ()

@property (nonatomic, readonly) HBSubtitlesDefaults *settings;

@property (nonatomic, readonly) HBLanguagesSelection *languagesList;
@property (nonatomic, unsafe_unretained) IBOutlet HBLanguageArrayController *tableController;

@property (nonatomic, weak) id<HBSubtitlesDefaultsControllerDelegate> delegate;

@end

@implementation HBSubtitlesDefaultsController

- (instancetype)initWithSettings:(HBSubtitlesDefaults *)settings delegate:(nonnull id<HBSubtitlesDefaultsControllerDelegate>)delegate
{
    self = [super initWithNibName:@"SubtitlesDefaults" bundle:nil];
    if (self)
    {
        _settings = settings;
        _languagesList = [[HBLanguagesSelection alloc] initWithLanguages:_settings.trackSelectionLanguages];
        _delegate = delegate;
        self.title = NSLocalizedString(@"Subtitles Selection Behaviour", @"Subtitles Selection Behaviour view title");
    }
    return self;
}

- (void)viewWillAppear
{
    self.settings.undo = self.view.window.undoManager;
    self.languagesList.undo = self.view.window.undoManager;
}

- (void)viewDidAppear
{
    self.view.window.preventsApplicationTerminationWhenModal = NO;
}

- (IBAction)ok:(id)sender
{
    self.settings.trackSelectionLanguages = [self.languagesList.selectedLanguages mutableCopy];
    [self.delegate subtitlesControllerDidEnd:self.settings returnCode:NSModalResponseOK];
}

- (IBAction)cancel:(id)sender
{
    [self.delegate subtitlesControllerDidEnd:self.settings returnCode:NSModalResponseCancel];
}

- (IBAction)openUserGuide:(id)sender
{
    [NSWorkspace.sharedWorkspace openURL:[HBUtilities.documentationBaseURL URLByAppendingPathComponent:@"advanced/audio-subtitle-defaults.html"]];
}

@end
