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
@property (nonatomic, unsafe_unretained) IBOutlet NSButton *showAllButton;

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

- (void)windowDidLoad
{
    [self addObserver:self forKeyPath:@"tableController.showSelectedOnly" options:0 context:HBSubtitlesDefaultsContext];

    if (self.settings.trackSelectionLanguages.count)
    {
        self.tableController.showSelectedOnly = YES;
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBSubtitlesDefaultsContext)
    {
        if ([keyPath isEqualToString:@"tableController.showSelectedOnly"])
        {
            [self.showAllButton setState:!self.tableController.showSelectedOnly];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (IBAction)edit:(id)sender
{
    self.tableController.showSelectedOnly = !self.tableController.showSelectedOnly;
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
    [[NSWorkspace sharedWorkspace] openURL:[HBUtilities.documentationURL URLByAppendingPathComponent:@"advanced/audio-subtitle-defaults.html"]];
}

- (void)dealloc
{
    @try {
        [self removeObserver:self forKeyPath:@"tableController.showSelectedOnly" context:HBSubtitlesDefaultsContext];
    } @catch (NSException * __unused exception) {}
}

@end
