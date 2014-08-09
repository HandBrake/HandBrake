/*  HBTreeNode.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

/**
 *  Notify a delegate that something changed in the tree.
 *  KVO observing a tree looked complicated an expensive, so this is a lightweight
 *  way to track the changes we need to know.
 */
@protocol HBTreeNodeDelegate <NSObject>

- (void)nodeDidChange;

@end

/**
 *  HBTreeNode
 */
@interface HBTreeNode : NSObject

// NSTreeController required properties
@property (nonatomic, readonly) NSMutableArray *children;
@property (nonatomic) BOOL isLeaf;

@property (nonatomic, assign) id<HBTreeNodeDelegate> delegate;

/**
 *  Executes a given block using each object in the tree, starting with the root object and continuing through the tree to the last object.
 *
 *  @param block The block to apply to elements in the tree.
 */
- (void)enumerateObjectsUsingBlock:(void (^)(id obj, NSIndexPath *idx, BOOL *stop))block;

// KVC Accessor Methods
- (NSUInteger)countOfChildren;
- (id)objectInChildrenAtIndex:(NSUInteger)index;
- (void)insertObject:(id)presetObject inChildrenAtIndex:(NSUInteger)index;
- (void)removeObjectFromChildrenAtIndex:(NSUInteger)index;

@end
