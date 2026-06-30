/*  HBPictureViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFiltersViewController.h"
#import "HBTableView.h"

@import HandBrakeKit;

@interface HBFiltersCellView : NSTableCellView

@property (nonatomic, weak) IBOutlet NSPopUpButton *presetsPopUpButton;
@property (nonatomic, weak) IBOutlet NSPopUpButton *tunesPopUpButton;

- (IBAction)remove:(id)sender;

@end

@implementation HBFiltersCellView

- (void)setObjectValue:(id)objectValue
{
    [super setObjectValue:objectValue];

    [self didChangeValueForKey:@"objectValue"];

    if ([objectValue isKindOfClass:[HBFilter class]])
    {
        HBFilter *filter = (HBFilter *)objectValue;

        HBFilterPresetTransformer *presetsTransformer = [[HBFilterPresetTransformer alloc] initWithWithFilterID:filter.filterID];
        HBFilterTuneTransformer   *tunesTransformer = [[HBFilterTuneTransformer alloc] initWithWithFilterID:filter.filterID];

        [self.presetsPopUpButton bind:@"selectedValue" toObject:self
                          withKeyPath:@"objectValue.preset"
                              options:@{NSValueTransformerBindingOption: presetsTransformer}];

        [self.tunesPopUpButton bind:@"selectedValue" toObject:self
                        withKeyPath:@"objectValue.tune"
                            options:@{NSValueTransformerBindingOption: tunesTransformer}];
    }
}

- (IBAction)remove:(id)sender
{
    if ([self.objectValue isKindOfClass:[HBFilter class]])
    {
        HBFilter *filter = (HBFilter *)self.objectValue;
        [filter.delegate removeFilter:filter];
    }
}

@end


@interface HBFiltersViewController ()

@property (nonatomic, readwrite) NSColor *labelColor;
@property (nonatomic, weak) IBOutlet NSPopUpButton *pullDown;
@property (nonatomic, weak) IBOutlet HBTableView *tableView;

@end

@implementation HBFiltersViewController

- (instancetype)init
{
    self = [super initWithNibName:@"HBFiltersViewController" bundle:nil];
    if (self)
    {
        _labelColor = [NSColor disabledControlTextColor];
    }
    return self;
}

- (NSMenuItem *)menuItemForFilterID:(int)filterID
{
    NSMenuItem *item = [[NSMenuItem alloc] init];
    item.title = [HBFilter localizedNameForFilterID:filterID];;
    item.tag = filterID;
    item.action = @selector(toggleFilter:);
    item.target = self;
    return item;
}

- (void)viewDidLoad
{
    for (HBFilterGroup *group in HBFilters.availableFilterGroups.reverseObjectEnumerator)
    {
        if (group.name.length)
        {
            NSMenuItem *container = [[NSMenuItem alloc] init];
            container.title = group.name;
            container.submenu = [[NSMenu alloc] init];

            for (NSNumber *filterID in group.filters)
            {
                NSMenuItem *item = [self menuItemForFilterID:filterID.intValue];
                [container.submenu addItem:item];
            }

            [self.pullDown.menu insertItem:container atIndex:1];
        }
        else
        {
            NSMenuItem *item = [self menuItemForFilterID:group.filters.firstObject.intValue];
            [self.pullDown.menu insertItem:item atIndex:1];
        }
    }
}

- (void)setFilters:(HBFilters *)filters
{
    _filters = filters;

    if (_filters)
    {
        self.labelColor = [NSColor controlTextColor];
    }
    else
    {
        self.labelColor = [NSColor disabledControlTextColor];
    }
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    if (menuItem.action == @selector(remove:))
    {
        return self.filters != nil && self.tableView.targetedRowIndexes.count;
    }
    if (menuItem.action == @selector(removeAll:))
    {
        return self.filters != nil && [self.filters countOfFilters];
    }

    if (menuItem.action == @selector(toggleFilter:))
    {
        int filterID = (int)menuItem.tag;
        menuItem.state = NSControlStateValueOff;

        for (HBFilter *filter in self.filters.filters)
        {
            if (filter.filterID == filterID)
            {
                menuItem.state = NSControlStateValueOn;
            }
        }
    }

    return YES;
}

- (IBAction)toggleFilter:(NSMenuItem *)sender
{
    int filterID = (int)sender.tag;

    if ([self.filters containsFilterWithID:filterID])
    {
        [self.filters removeFilterWithID:filterID];
    }
    else
    {
        HBFilterGroup *group = [self.filters groupWithFilterID:filterID];

        if (group && [self.filters containsFilterWithIDs:group.filters])
        {
            for (NSNumber *groupFilterID in group.filters)
            {
                [self.filters removeFilterWithID:groupFilterID.intValue];
            }
        }

        [self.filters addFilterWithID:filterID];
    }
}

- (IBAction)remove:(id)sender
{
    [self.filters removeFiltersAtIndexes:self.tableView.targetedRowIndexes];
}

- (IBAction)removeAll:(id)sender
{
    [self.filters removeAll];
}

@end
