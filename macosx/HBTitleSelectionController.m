/* HBTitleSelectionController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitleSelectionController.h"
#import "HBTitle.h"

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

@property (nonatomic, readwrite) NSArray<HBTitleSelection *> *titles;
@property (nonatomic, readonly, assign) id<HBTitleSelectionDelegate> delegate;

@end

@implementation HBTitleSelectionController

- (instancetype)initWithTitles:(NSArray *)titles delegate:(id<HBTitleSelectionDelegate>)delegate
{
    self = [super initWithWindowNibName:@"HBTitleSelection"];
    if (self)
    {
        _delegate = delegate;

        NSMutableArray<HBTitleSelection *> *array = [[NSMutableArray alloc] init];
        for (HBTitle *title in titles)
        {
            [array addObject:[[HBTitleSelection alloc] initWithTitle:title undo:self.window.undoManager]];
        }
        self.titles = [array copy];
    }

    return self;
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
    NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];

    [self.titles enumerateObjectsUsingBlock:^(HBTitleSelection *obj, NSUInteger idx, BOOL *stop) {
        if (obj.selected)
        {
            [indexes addIndex:obj.title.index];
        }
    }];
    [self.delegate didSelectIndexes:indexes];
}

- (IBAction)cancel:(id)sender
{
    [self.delegate didSelectIndexes:[NSIndexSet indexSet]];
}

@end
