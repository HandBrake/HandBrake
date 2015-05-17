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

- (id)copyWithZone:(NSZone *)zone
{
    HBTreeNode *node = [[self class] allocWithZone:zone];
    node->_children = [[NSMutableArray alloc] init];
    node->_isLeaf = self.isLeaf;

    for (HBTreeNode *children in self.children)
    {
        [node.children addObject:[children copy]];
    }

    return node;
}

- (NSUInteger)countOfChildren
{
    return self.children.count;
}

- (id)objectInChildrenAtIndex:(NSUInteger)index
{
    return (self.children)[index];
}

- (void)insertObject:(HBTreeNode *)presetObject inChildrenAtIndex:(NSUInteger)index
{
    [self.children insertObject:presetObject atIndex:index];
    [presetObject setDelegate:self.delegate];

    for (HBTreeNode *node in self.children)
    {
        node.delegate = self.delegate;
    }

    [self.delegate nodeDidChange:self];
}

- (void)removeObjectFromChildrenAtIndex:(NSUInteger)index
{
    [self.children removeObjectAtIndex:index];
    [self.delegate nodeDidChange:self];
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

- (NSIndexPath *)indexPathOfObject:(id)obj
{
    __block NSIndexPath *retValue = nil;

    // Visit the whole tree to find the index path.
    [self enumerateObjectsUsingBlock:^(id obj2, NSIndexPath *idx, BOOL *stop)
    {
         if ([obj2 isEqualTo:obj])
         {
             retValue = idx;
             *stop = YES;
         }
     }];

    return retValue;
}

- (void)removeObjectAtIndexPath:(NSIndexPath *)idx
{
    HBTreeNode *parentNode = self;

    // Find the object parent array
    // and delete it.
    NSUInteger currIdx = 0;
    NSUInteger i = 0;
    for (i = 0; i < idx.length - 1; i++)
    {
        currIdx = [idx indexAtPosition:i];

        if (parentNode.children.count > currIdx)
        {
            parentNode = (parentNode.children)[currIdx];
        }
    }

    currIdx = [idx indexAtPosition:i];

    if (parentNode.children.count > currIdx)
    {
        id removedNode = parentNode.children[currIdx];

        [parentNode removeObjectFromChildrenAtIndex:currIdx];

        if ([self.delegate respondsToSelector:@selector(treeDidRemoveNode:)])
        {
            [self.delegate treeDidRemoveNode:removedNode];
        }
    }
}

@end
