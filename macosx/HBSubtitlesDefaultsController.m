/*  HBSubtitlesDefaultsController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesDefaultsController.h"
#import "HBSubtitlesSettings.h"
#import "HBLanguagesSelection.h"

@interface HBSubtitlesDefaultsController ()

@property (nonatomic, readonly) HBSubtitlesSettings *settings;

@property (nonatomic, readonly) HBLanguagesSelection *languagesList;
@property (assign) IBOutlet HBLanguageArrayController *tableController;
@property (assign) IBOutlet NSButton *showAllButton;

@end

@implementation HBSubtitlesDefaultsController

- (instancetype)initWithSettings:(HBSubtitlesSettings *)settings
{
    self = [super initWithWindowNibName:@"SubtitlesDefaults"];
    if (self)
    {
        _settings = [settings retain];
        _languagesList = [[HBLanguagesSelection alloc] initWithLanguages:_settings.trackSelectionLanguages];
    }
    return self;
}

- (void)windowDidLoad
{
    if (self.settings.trackSelectionLanguages.count)
    {
        self.tableController.showSelectedOnly = YES;
        [self.showAllButton setState:NSOffState];
    }
}

- (IBAction)edit:(id)sender
{
    self.tableController.showSelectedOnly = !self.tableController.showSelectedOnly;
}

- (IBAction)done:(id)sender
{
    [[self window] orderOut:nil];
    [NSApp endSheet:[self window]];

    [self.settings.trackSelectionLanguages removeAllObjects];
    [self.settings.trackSelectionLanguages addObjectsFromArray:self.languagesList.selectedLanguages];

    if ([self.delegate respondsToSelector:@selector(sheetDidEnd)])
    {
        [self.delegate performSelector:@selector(sheetDidEnd)];
    }
}

- (void)dealloc
{
    [super dealloc];
    [_settings release];
}

@end
