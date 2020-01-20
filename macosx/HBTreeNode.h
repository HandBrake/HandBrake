/*  HBTreeNode.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class HBTreeNode;

/**
 *  Notify a delegate that something changed in the tree.
 *  KVO observing a tree looks complicated and expensive, so this is a lightweight
 *  way to track the changes we need to know.
 */
@protocol HBTreeNodeDelegate <NSObject>

- (void)nodeDidChange:(HBTreeNode *)node;

@optional
- (void)treeDidRemoveNode:(HBTreeNode *)node;

@end

/**
 *  HBTreeNode
 */
@interface HBTreeNode : NSObject <NSCopying>

// NSTreeController required properties
@property (nonatomic, readonly) NSMutableArray<HBTreeNode *> *children;
@property (nonatomic) BOOL isLeaf;

@property (nonatomic, weak) id<HBTreeNodeDelegate> delegate;

/**
 *  Executes a given block using each object in the tree, starting with the root object and continuing through the tree to the last object.
 *
 *  @param block The block to apply to elements in the tree.
 */
- (void)enumerateObjectsUsingBlock:(void (^)(id obj, NSIndexPath *idx, BOOL *stop))block;

/**
 *  Returns the index path of an object in the tree.
 *
 *  @param obj the object of the wanted NSIndexPath
 *
 *  @return The index path whose corresponding value is equal to the preset. Returns nil if not found.
 */
- (nullable NSIndexPath *)indexPathOfObject:(id)obj;

/**
 *  Removes the object at the specified index path.
 *
 *  @param idx the NSIndexPath of the object to delete.
 */
- (void)removeObjectAtIndexPath:(NSIndexPath *)idx;

/// Replaces the object at the specified index path.
/// @param idx the NSIndexPath of the object to replace
/// @param object the new object
- (void)replaceObjectAtIndexPath:(NSIndexPath *)idx withObject:(HBTreeNode *)object;

// KVC Accessor Methods

@property (nonatomic, readonly) NSUInteger countOfChildren;
- (id)objectInChildrenAtIndex:(NSUInteger)index;
- (void)insertObject:(HBTreeNode *)presetObject inChildrenAtIndex:(NSUInteger)index;
- (void)replaceObjectInChildrenAtIndex:(NSUInteger)index withObject:(HBTreeNode *)object;
- (void)removeObjectFromChildrenAtIndex:(NSUInteger)index;

@end

NS_ASSUME_NONNULL_END

