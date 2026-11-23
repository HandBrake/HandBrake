/*  HBQueueImportWindowController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueImportWindowController.h"
#import "HBQueueJobItem.h"

@interface HBQueueImportWindowController () <NSTableViewDataSource, NSTableViewDelegate>

@property (nonatomic) NSArray<id<HBQueueItem>> *items;
@property (nonatomic) NSMutableIndexSet *selectedIndexes;
@property (nonatomic, weak) IBOutlet NSTableView *tableView;
@property (nonatomic, weak) IBOutlet NSButton *importButton;

@end

@implementation HBQueueImportWindowController

- (instancetype)initWithItems:(NSArray<id<HBQueueItem>> *)items
{
    self = [super initWithWindowNibName:@"HBQueueImportWindowController"];
    if (self)
    {
        _items = [items copy];
        _selectedIndexes = [NSMutableIndexSet indexSetWithIndexesInRange:NSMakeRange(0, items.count)];
    }
    return self;
}

- (NSArray<id<HBQueueItem>> *)selectedItems
{
    return [self.items objectsAtIndexes:self.selectedIndexes];
}

- (IBAction)toggleItem:(NSButton *)sender
{
    NSInteger row = [self.tableView rowForView:sender];
    if (row >= 0)
    {
        if (sender.state == NSControlStateValueOn)
        {
            [self.selectedIndexes addIndex:row];
        }
        else
        {
            [self.selectedIndexes removeIndex:row];
        }
        self.importButton.enabled = self.selectedIndexes.count > 0;
    }
}

/// The sender's tag tells whether to select all the items or none.
- (IBAction)selectItems:(NSButton *)sender
{
    [self.selectedIndexes removeAllIndexes];
    if (sender.tag)
    {
        [self.selectedIndexes addIndexesInRange:NSMakeRange(0, self.items.count)];
    }
    self.importButton.enabled = self.selectedIndexes.count > 0;
    [self.tableView reloadData];
}

- (IBAction)cancel:(id)sender
{
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseCancel];
}

- (IBAction)import:(id)sender
{
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseOK];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return self.items.count;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row
{
    id<HBQueueItem> item = self.items[row];
    NSTableCellView *cell = [tableView makeViewWithIdentifier:column.identifier owner:self];

    if ([column.identifier isEqualToString:@"title"])
    {
        NSButton *checkBox = cell.subviews.firstObject;
        checkBox.title = item.title;
        checkBox.state = [self.selectedIndexes containsIndex:row] ? NSControlStateValueOn : NSControlStateValueOff;
    }
    else
    {
        NSString *path = [item isKindOfClass:[HBQueueJobItem class]] ? [(HBQueueJobItem *)item destinationFolderURL].path : nil;
        cell.textField.stringValue = path ?: @"";
        cell.toolTip = path;
    }

    return cell;
}

@end
