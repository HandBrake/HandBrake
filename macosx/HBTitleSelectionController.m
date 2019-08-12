/* HBTitleSelectionController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitleSelectionController.h"

@import HandBrakeKit;

@interface HBTitleSelection : NSObject
@property (nonatomic, readonly) HBTitle *title;
@property (nonatomic, readonly) BOOL selected;
@property (nonatomic, readonly, assign) NSUndoManager *undo;
@end

@implementation HBTitleSelection
- (instancetype)initWithTitle:(HBTitle *)title undo:(NSUndoManager *)undo
{
    if (self = [super init])
    {
        _title = title;
        _selected = YES;
        _undo = undo;
    }
    return self;
}

- (void)setSelected:(BOOL)selected
{
    if (selected != _selected)
    {
        [[self.undo prepareWithInvocationTarget:self] setSelected:_selected];
    }
    _selected = selected;
}
@end

@interface HBTitleSelectionController () <NSTableViewDataSource, NSTableViewDelegate>

@property (nonatomic, strong) IBOutlet NSArrayController *arrayController;
@property (nonatomic, readwrite) NSArray<HBTitleSelection *> *titles;
@property (nonatomic, readonly, assign) id<HBTitleSelectionDelegate> delegate;
@property (nonatomic, readonly) NSString *message;

@end

@implementation HBTitleSelectionController

- (instancetype)initWithTitles:(NSArray<HBTitle *> *)titles presetName:(NSString *)presetName delegate:(id<HBTitleSelectionDelegate>)delegate
{
    self = [super initWithWindowNibName:@"HBTitleSelection"];
    if (self)
    {
        _delegate = delegate;
        _message = [NSString stringWithFormat:NSLocalizedString(@"Select the titles to add to the queue using the %@ preset:" , @"Titles selection sheet -> informative text"), presetName];

        NSMutableArray<HBTitleSelection *> *array = [[NSMutableArray alloc] init];
        for (HBTitle *title in titles)
        {
            [array addObject:[[HBTitleSelection alloc] initWithTitle:title undo:self.window.undoManager]];
        }
        self.titles = [array copy];
    }

    return self;
}

- (void)windowDidLoad
{
    NSSortDescriptor *mySortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"title.index" ascending:YES];
    self.arrayController.sortDescriptors = [NSArray arrayWithObject:mySortDescriptor];
}

- (IBAction)deselectAll:(id)sender
{
    for (HBTitleSelection *title in self.titles)
    {
        title.selected = NO;
    }
}

- (IBAction)add:(id)sender
{
    NSMutableArray<HBTitle *> *titles = [NSMutableArray array];
    [self.arrayController.arrangedObjects enumerateObjectsUsingBlock:^(HBTitleSelection *obj, NSUInteger idx, BOOL *stop) {
        if (obj.selected)
        {
            [titles addObject:obj.title];
        }
    }];
    [self.delegate didSelectTitles:titles];
}

- (IBAction)cancel:(id)sender
{
    [self.delegate didSelectTitles:@[]];
}

@end

@interface HBTitleSelectionController (TouchBar) <NSTouchBarProvider, NSTouchBarDelegate>
@end

@implementation HBTitleSelectionController (TouchBar)

@dynamic touchBar;

static NSTouchBarItemIdentifier HBTouchBarGroup = @"fr.handbrake.buttonsGroup";
static NSTouchBarItemIdentifier HBTouchBarAdd = @"fr.handbrake.openSource";
static NSTouchBarItemIdentifier HBTouchBarCancel = @"fr.handbrake.addToQueue";

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.defaultItemIdentifiers = @[HBTouchBarGroup];
    bar.principalItemIdentifier = HBTouchBarGroup;

    return bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualTo:HBTouchBarGroup])
    {
        NSCustomTouchBarItem *cancelItem = [[NSCustomTouchBarItem alloc] initWithIdentifier:HBTouchBarAdd];
        cancelItem.customizationLabel = NSLocalizedString(@"Cancel", @"Touch bar");
        NSButton *cancelButton = [NSButton buttonWithTitle:NSLocalizedString(@"Cancel", @"Touch bar") target:self action:@selector(cancel:)];
        [cancelButton.widthAnchor constraintGreaterThanOrEqualToConstant:160].active = YES;
        cancelItem.view = cancelButton;

        NSCustomTouchBarItem *addItem = [[NSCustomTouchBarItem alloc] initWithIdentifier:HBTouchBarCancel];
        addItem.customizationLabel = NSLocalizedString(@"Add To Queue", @"Touch bar");
        NSButton *addButton = [NSButton buttonWithTitle:NSLocalizedString(@"Add To Queue", @"Touch bar") target:self action:@selector(add:)];
        [addButton.widthAnchor constraintGreaterThanOrEqualToConstant:160].active = YES;
        addButton.keyEquivalent = @"\r";
        addItem.view = addButton;

        NSGroupTouchBarItem *item = [NSGroupTouchBarItem groupItemWithIdentifier:identifier items:@[cancelItem, addItem]];
        return item;
    }

    return nil;
}

@end
