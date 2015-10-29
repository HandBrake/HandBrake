/*  HBAudioDefaultsController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioDefaultsController.h"
#import "HBAudioDefaults.h"
#import "HBLanguagesSelection.h"

static void *HBAudioDefaultsContext = &HBAudioDefaultsContext;

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
        _settings.undo = self.window.undoManager;
        _languagesList.undo = self.window.undoManager;
    }
    return self;
}

- (void)windowDidLoad
{
    [self addObserver:self forKeyPath:@"tableController.showSelectedOnly" options:0 context:HBAudioDefaultsContext];

    if (self.settings.trackSelectionLanguages.count)
    {
        self.tableController.showSelectedOnly = YES;
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBAudioDefaultsContext)
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

- (IBAction)ok:(id)sender
{
    self.settings.trackSelectionLanguages = [self.languagesList.selectedLanguages mutableCopy];
    [self.window orderOut:nil];
    [NSApp endSheet:self.window returnCode:NSModalResponseOK];
}

- (IBAction)cancel:(id)sender
{
    [self.window orderOut:nil];
    [NSApp endSheet:self.window returnCode:NSModalResponseCancel];
}

- (void)dealloc
{
    @try {
        [self removeObserver:self forKeyPath:@"tableController.showSelectedOnly" context:HBAudioDefaultsContext];
    } @catch (NSException * __unused exception) {}

}

@end
