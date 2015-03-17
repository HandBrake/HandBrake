/*  HBAudioDefaultsController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioDefaultsController.h"
#import "HBAudioDefaults.h"
#import "HBLanguagesSelection.h"

static void *HBAudioDefaultsContex = &HBAudioDefaultsContex;

@interface HBAudioDefaultsController ()

@property (nonatomic, readonly, strong) HBAudioDefaults *settings;

@property (nonatomic, readonly, strong) HBLanguagesSelection *languagesList;

@property (unsafe_unretained) IBOutlet HBLanguageArrayController *tableController;
@property (unsafe_unretained) IBOutlet NSButton *showAllButton;

@property (unsafe_unretained) IBOutlet NSArrayController *tracksController;

@end

@implementation HBAudioDefaultsController

- (instancetype)initWithSettings:(HBAudioDefaults *)settings
{
    self = [super initWithWindowNibName:@"AudioDefaults"];
    if (self)
    {
        _settings = settings;
        _languagesList = [[HBLanguagesSelection alloc] initWithLanguages:_settings.trackSelectionLanguages];
    }
    return self;
}

- (void)windowDidLoad
{
    [self addObserver:self forKeyPath:@"tableController.showSelectedOnly" options:0 context:HBAudioDefaultsContex];

    if (self.settings.trackSelectionLanguages.count)
    {
        self.tableController.showSelectedOnly = YES;
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBAudioDefaultsContex)
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

- (IBAction)addTrack:(id)sender
{
    if ([sender selectedSegment])
    {
        if ([[self.tracksController arrangedObjects] count] && self.tracksController.selectionIndex != NSNotFound)
        {
            [self.tracksController removeObjectAtArrangedObjectIndex:self.tracksController.selectionIndex];
        }
    }
    else
    {
        [self.settings addTrack];
    }
}

- (IBAction)done:(id)sender
{
    [[self window] orderOut:nil];
    [NSApp endSheet:[self window]];

    [self.settings.trackSelectionLanguages removeAllObjects];
    [self.settings.trackSelectionLanguages addObjectsFromArray:self.languagesList.selectedLanguages];
}

- (void)dealloc
{

    @try {
        [self removeObserver:self forKeyPath:@"tableController.showSelectedOnly"];
    } @catch (NSException * __unused exception) {}

}

@end
