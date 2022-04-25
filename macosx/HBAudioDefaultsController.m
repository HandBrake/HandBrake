/*  HBAudioDefaultsController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioDefaultsController.h"
#import "HBLanguagesSelection.h"

@import HandBrakeKit;

static void *HBAudioDefaultsContext = &HBAudioDefaultsContext;

@interface HBAudioDefaultsController ()

@property (nonatomic, readonly, strong) HBAudioDefaults *settings;

@property (nonatomic, readonly, strong) HBLanguagesSelection *languagesList;

@property (nonatomic, unsafe_unretained) IBOutlet HBLanguageArrayController *tableController;

@property (nonatomic, unsafe_unretained) IBOutlet NSArrayController *tracksController;

@end

@implementation HBAudioDefaultsController

- (instancetype)initWithSettings:(HBAudioDefaults *)settings
{
    self = [super initWithWindowNibName:@"AudioDefaults"];
    if (self)
    {
        _settings = settings;
        _languagesList = [[HBLanguagesSelection alloc] initWithLanguages:_settings.trackSelectionLanguages];
        _settings.undo = self.window.undoManager;
        _languagesList.undo = self.window.undoManager;
    }
    return self;
}

- (IBAction)addTrack:(id)sender
{
    if ([sender selectedSegment])
    {
        if ([self.tracksController.arrangedObjects count] && self.tracksController.selectionIndex != NSNotFound)
        {
            [self.tracksController removeObjectAtArrangedObjectIndex:self.tracksController.selectionIndex];
        }
    }
    else
    {
        [self.settings addTrack];
    }
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
