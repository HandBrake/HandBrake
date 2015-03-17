/* HBTitleSelectionController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitleSelectionController.h"
#import "HBTitle.h"

@interface HBTitleSelectionController () <NSTableViewDataSource, NSTableViewDelegate>

@property (nonatomic, readonly) NSArray *titles;
@property (nonatomic, readonly) NSMutableArray *selection;

@property (nonatomic, readonly, unsafe_unretained) id<HBTitleSelectionDelegate> delegate;

@end

@implementation HBTitleSelectionController

- (instancetype)initWithTitles:(NSArray *)titles delegate:(id<HBTitleSelectionDelegate>)delegate
{
    self = [super initWithWindowNibName:@"HBTitleSelection"];
    if (self)
    {
        _titles = titles;
        _selection = [[NSMutableArray alloc] initWithCapacity:titles.count];
        _delegate = delegate;

        for (NSUInteger i = 0; i < titles.count; i++)
        {
            _selection[i] = @YES;
        }
    }

    return self;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return self.titles.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    HBTitle *title = self.titles[row];

    if ([tableColumn.identifier isEqualTo:@"index"])
    {
        return @(title.index);
    }
    else if ([tableColumn.identifier isEqualTo:@"title"])
    {
        return self.selection[row];
    }
    else if ([tableColumn.identifier isEqualTo:@"duration"])
    {
        return title.timeCode;
    }

    return nil;
}

- (void)tableView:(NSTableView *)tableView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row;
{
    if ([tableColumn.identifier isEqualTo:@"title"])
    {
        self.selection[row] = object;
    }
}

- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    if ([tableColumn.identifier isEqualTo:@"title"])
    {
        HBTitle *title = self.titles[row];
        [aCell setTitle:title.name];
    }
}

- (IBAction)add:(id)sender
{
    NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];

    [self.selection enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        if ([obj boolValue])
        {
            HBTitle *title = self.titles[idx];
            [indexes addIndex:title.index];
        }

    }];
    [self.delegate didSelectIndexes:indexes];
}

- (IBAction)cancel:(id)sender
{
    [self.delegate didSelectIndexes:[NSIndexSet indexSet]];
}

@end
