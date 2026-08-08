/*  HBTrackTitleViewController.m

This file is part of the HandBrake source code.
Homepage: <http://handbrake.fr/>.
It may be used under the terms of the GNU General Public License. */

#import "HBTrackTitleViewController.h"
#import "HBTableView.h"

@import HandBrakeKit;

@interface HBTrackTitleViewController ()

@property (nonatomic, weak) IBOutlet NSPopUpButton *pullDown;
@property (nonatomic, weak) IBOutlet HBTableView *tableView;

@end

@implementation HBTrackTitleViewController

- (void)viewDidLoad
{
    HBFilters *filters = [[HBAudioFilters alloc] init];

    for (NSNumber *filterID in filters.availableFilters.reverseObjectEnumerator)
    {
        NSMenuItem *item = [[NSMenuItem alloc] init];
        item.title = [HBFilter localizedNameForFilterID:filterID.intValue];
        item.tag = filterID.intValue;
        item.action = @selector(toggleFilter:);
        item.target = self;

        [self.pullDown.menu insertItem:item atIndex:1];
    }
}

- (HBAudioFilters *)filters
{
    return (HBAudioFilters *)[self.track filters];
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
