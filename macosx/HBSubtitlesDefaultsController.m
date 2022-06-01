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

@end

@implementation HBSubtitlesDefaultsController

- (instancetype)initWithSettings:(HBSubtitlesDefaults *)settings
{
    self = [super initWithWindowNibName:@"SubtitlesDefaults"];
    if (self)
    {
        _settings = settings;
        _languagesList = [[HBLanguagesSelection alloc] initWithLanguages:_settings.trackSelectionLanguages];
        _settings.undo = self.window.undoManager;
    }
    return self;
}

- (IBAction)ok:(id)sender
{
    self.settings.trackSelectionLanguages = [self.languagesList.selectedLanguages mutableCopy];
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseOK];
}

- (IBAction)cancel:(id)sender
{
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseCancel];
}

- (IBAction)openUserGuide:(id)sender
{
    [NSWorkspace.sharedWorkspace openURL:[HBUtilities.documentationBaseURL URLByAppendingPathComponent:@"advanced/audio-subtitle-defaults.html"]];
}

@end
