/*  HBAudioDefaultsController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioDefaultsController.h"
#import "HBLanguagesSelection.h"
#import "HBAudioFiltersViewController.h"

@import HandBrakeKit;

static void *HBAudioDefaultsContext = &HBAudioDefaultsContext;

@interface HBAudioDefaultsController ()

@property (nonatomic, readonly, strong) HBAudioDefaults *settings;

@property (nonatomic, readonly, strong) HBLanguagesSelection *languagesList;

@property (nonatomic, unsafe_unretained) IBOutlet HBLanguageArrayController *tableController;

@property (nonatomic, unsafe_unretained) IBOutlet NSArrayController *tracksController;
@property (nonatomic, weak) IBOutlet NSSegmentedControl *tracksControl;
@property (nonatomic, weak) IBOutlet NSTableView *table;

@property (nonatomic, weak) id<HBAudioDefaultsControllerDelegate> delegate;

@end

@implementation HBAudioDefaultsController

- (instancetype)initWithSettings:(HBAudioDefaults *)settings delegate:(id<HBAudioDefaultsControllerDelegate>)delegate
{
    self = [super initWithNibName:@"AudioDefaults" bundle:nil];
    if (self)
    {
        _settings = settings;
        _languagesList = [[HBLanguagesSelection alloc] initWithLanguages:_settings.trackSelectionLanguages];
        _delegate = delegate;
    }
    return self;
}

- (void)viewDidLoad
{
    [self.tracksController addObserver:self
                            forKeyPath:@"selectedObjects"
                               options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                               context:HBAudioDefaultsContext];
}

- (void)viewWillAppear
{
    self.settings.undo = self.view.window.undoManager;
    self.languagesList.undo = self.view.window.undoManager;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBAudioDefaultsContext)
    {
        if ([keyPath isEqualToString:@"selectedObjects"])
        {
            BOOL selected = self.tracksController.selectedObjects.count > 0;
            [self.tracksControl setEnabled:selected forSegment:1];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (IBAction)addTrack:(id)sender
{
    if ([sender selectedSegment])
    {
        if ([self.tracksController.arrangedObjects count] && self.tracksController.selectionIndex != NSNotFound)
        {
            [self.tracksController removeObjectsAtArrangedObjectIndexes:self.tracksController.selectionIndexes];
        }
    }
    else
    {
        [self.settings addTrack];
    }
}

- (IBAction)showFiltersPopOver:(id)sender
{
    HBAudioFiltersViewController *controller = [[HBAudioFiltersViewController alloc] init];
    NSInteger index = [self.table rowForView:sender];
    if (index != -1)
    {
        controller.track = self.settings.tracksArray[index];
        [self presentViewController:controller
            asPopoverRelativeToRect:[sender bounds]
                             ofView:sender
                      preferredEdge:NSRectEdgeMaxY
                           behavior:NSPopoverBehaviorSemitransient];
    }
}

- (IBAction)ok:(id)sender
{
    self.settings.trackSelectionLanguages = [self.languagesList.selectedLanguages mutableCopy];
    [self.delegate audioControllerDidEnd:self.settings returnCode:NSModalResponseOK];
    [self dismissViewController:self];
}

- (IBAction)cancel:(id)sender
{
    [self.delegate audioControllerDidEnd:self.settings returnCode:NSModalResponseCancel];
    [self dismissViewController:self];
}

- (IBAction)openUserGuide:(id)sender
{
    [NSWorkspace.sharedWorkspace openURL:[HBUtilities.documentationBaseURL URLByAppendingPathComponent:@"advanced/audio-subtitle-defaults.html"]];
}

@end
