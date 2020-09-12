/*  NSArray+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSArray (HBAdditions)

- (BOOL)HB_allSatisfy:(BOOL (^)(id object))block;
- (BOOL)HB_containsWhere:(BOOL (^)(id object))block;
- (NSArray *)HB_filteredArrayUsingBlock:(BOOL (^)(id object))block;
- (NSIndexSet *)HB_indexesOfObjectsUsingBlock:(BOOL (^)(id object))block;

@end

@interface NSIndexSet (HBAdditions)

- (NSIndexSet *)HB_unionWith:(NSIndexSet *)indexSet;
- (NSIndexSet *)HB_intersectionWith:(NSIndexSet *)indexSet;

@end

NS_ASSUME_NONNULL_END

