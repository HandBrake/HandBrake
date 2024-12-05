/* HBTitleSelectionController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitleSelectionController.h"
#import "HBTitleSelectionRangeController.h"
#import "HBTableView.h"

@import HandBrakeKit;

@interface HBTitleSelection : NSObject

@property (nonatomic, readonly) HBTitle *title;
@property (nonatomic, readonly) BOOL enabled;
@property (nonatomic, readonly, assign) NSUndoManager *undo;

@end

@implementation HBTitleSelection

- (instancetype)initWithTitle:(HBTitle *)title undo:(NSUndoManager *)undo
{
    if (self = [super init])
    {
        _title = title;
        _enabled = YES;
        _undo = undo;
    }
    return self;
}

- (void)setEnabled:(BOOL)enabled
{
    if (enabled != _enabled)
    {
        [[self.undo prepareWithInvocationTarget:self] setEnabled:_enabled];
    }
    _enabled = enabled;
}

@end

@interface HBTitleSelectionController () <NSTableViewDataSource, NSTableViewDelegate, NSMenuItemValidation>

@property (nonatomic, weak) IBOutlet HBTableView *tableView;
@property (nonatomic, weak) IBOutlet NSView *rangeView;

@property (nonatomic, strong) IBOutlet NSArrayController *arrayController;

@property (nonatomic, readonly) HBTitleSelectionRangeController *selectionRangeController;

@property (nonatomic, readwrite) NSArray<HBTitleSelection *> *titles;
@property (nonatomic, readonly, weak) id<HBTitleSelectionDelegate> delegate;

@property (nonatomic, readonly) NSString *message;

@end

@implementation HBTitleSelectionController

- (instancetype)initWithTitles:(NSArray<HBTitle *> *)titles presetName:(NSString *)presetName delegate:(id<HBTitleSelectionDelegate>)delegate
{
    self = [super initWithWindowNibName:@"HBTitleSelection"];
    if (self)
    {
        _selectionRangeController = [[HBTitleSelectionRangeController alloc] initWithTitles:titles];
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

    self.selectionRangeController.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.selectionRangeController.view.frame = NSMakeRect(0, 0, self.rangeView.frame.size.width, self.rangeView.frame.size.height);
    [self.rangeView addSubview:self.selectionRangeController.view];

    self.selectionRangeController.range.undo = self.window.undoManager;
}

- (IBAction)enable:(id)sender
{
    NSIndexSet *indexes = self.tableView.targetedRowIndexes;
    for (HBTitleSelection *title in [self.arrayController.arrangedObjects objectsAtIndexes:indexes])
    {
        title.enabled = YES;
    }
}

- (IBAction)disable:(id)sender
{
    NSIndexSet *indexes = self.tableView.targetedRowIndexes;
    for (HBTitleSelection *title in [self.arrayController.arrangedObjects objectsAtIndexes:indexes])
    {
        title.enabled = NO;
    }
}

- (IBAction)enableAll:(id)sender
{
    for (HBTitleSelection *title in self.titles)
    {
        title.enabled = YES;
    }
}

- (IBAction)disableAll:(id)sender
{
    for (HBTitleSelection *title in self.titles)
    {
        title.enabled = NO;
    }
}

- (IBAction)invertState:(id)sender
{
    for (HBTitleSelection *title in self.titles)
    {
        title.enabled = !title.enabled;
    }
}

- (IBAction)add:(id)sender
{
    NSMutableArray<HBTitle *> *titles = [NSMutableArray array];
    [self.arrayController.arrangedObjects enumerateObjectsUsingBlock:^(HBTitleSelection *obj, NSUInteger idx, BOOL *stop) {
        if (obj.enabled)
        {
            [titles addObject:obj.title];
        }
    }];

    HBTitleSelectionRange *range = self.selectionRangeController.enabled ?
                                    [self.selectionRangeController.range copy] : nil;

    [self.delegate didSelectTitles:titles range:range];
}

- (IBAction)cancel:(id)sender
{
    [self.delegate didSelectTitles:@[] range:nil];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    SEL action = menuItem.action;

    if (action == @selector(enable:) || action == @selector(disable:))
    {
        NSIndexSet *targetedRowIndexes = self.tableView.targetedRowIndexes;
        if (targetedRowIndexes.count == 1)
        {
            NSUInteger index = targetedRowIndexes.firstIndex;
            HBTitleSelection *title = [self.arrayController.arrangedObjects objectAtIndex:index];
            if ((action == @selector(enable:) && title.enabled) ||
                (action == @selector(disable:) && !title.enabled))
            {
                return NO;
            }
        }
        else
        {
            return targetedRowIndexes.count > 0;
        }
    }

    return YES;
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
