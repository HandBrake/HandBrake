/*  HBTreeNode.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTreeNode.h"

@implementation HBTreeNode

- (instancetype)init
{
    self = [super init];
    if (self) {
        _children = [[NSMutableArray alloc] init];
        _isLeaf = YES;
    }
    return self;
}

- (NSUInteger)countOfChildren
{
    return self.children.count;
}

- (id)objectInChildrenAtIndex:(NSUInteger)index
{
    return [self.children objectAtIndex:index];
}

- (void)insertObject:(HBTreeNode *)presetObject inChildrenAtIndex:(NSUInteger)index
{
    [self.children insertObject:presetObject atIndex:index];
    [presetObject setDelegate:self.delegate];

    [self.delegate nodeDidChange];
}

- (void)removeObjectFromChildrenAtIndex:(NSUInteger)index
{
    [self.children removeObjectAtIndex:index];
    [self.delegate nodeDidChange];
}

#pragma mark - Enumeration

- (void)enumerateObjectsUsingBlock:(void (^)(id obj, NSIndexPath *idx, BOOL *stop))block
{
    BOOL stop = NO;
    NSMutableArray *queue = [[NSMutableArray alloc] init];
    NSMutableArray *indexesQueue = [[NSMutableArray alloc] init];

    [queue addObject:self];
    [indexesQueue addObject:[[NSIndexPath alloc] init]];

    HBTreeNode *node = nil;
    while ((node = [queue lastObject]) != nil)
    {
        // Get the index path of the current object
        NSIndexPath *indexPath = [indexesQueue lastObject];

        // Call the block
        block(node, indexPath, &stop);

        if (stop)
        {
            break;
        }

        [indexesQueue removeLastObject];

        for (NSInteger i = node.children.count - 1; i >= 0; i--)
        {
            [indexesQueue addObject:[indexPath indexPathByAddingIndex:i]];
        }

        [queue removeLastObject];

        for (id childNode in [node.children reverseObjectEnumerator])
        {
            [queue addObject:childNode];
        }
    }
}

@end
