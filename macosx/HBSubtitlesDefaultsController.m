/*  HBSubtitlesDefaultsController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesDefaultsController.h"
#import "HBSubtitlesSettings.h"
#import "HBLanguagesSelection.h"

static void *HBSubtitlesDefaultsContex = &HBSubtitlesDefaultsContex;

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
    [self addObserver:self forKeyPath:@"tableController.showSelectedOnly" options:0 context:HBSubtitlesDefaultsContex];

    if (self.settings.trackSelectionLanguages.count)
    {
        self.tableController.showSelectedOnly = YES;
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBSubtitlesDefaultsContex)
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
    [_settings release];

    @try {
        [self removeObserver:self forKeyPath:@"tableController.showSelectedOnly"];
    } @catch (NSException * __unused exception) {}

    [super dealloc];
}

@end
