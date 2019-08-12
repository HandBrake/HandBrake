/*  NSArray+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSArray (HBAdditions)

- (NSArray *)filteredArrayUsingBlock:(BOOL (^)(id object))block;
- (NSIndexSet *)indexesOfObjectsUsingBlock:(BOOL (^)(id object))block;

@end

NS_ASSUME_NONNULL_END

