/*  NSArray+HBAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "NSArray+HBAdditions.h"

@implementation NSArray (HBAdditions)

- (NSArray *)filteredArrayUsingBlock:(BOOL (^)(id object))block
{
    NSMutableArray *filteredArray = [NSMutableArray array];
    for (id object in self)
    {
        if (block(object))
        {
            [filteredArray addObject:object];
        }
    }
    return [filteredArray copy];
}

- (NSIndexSet *)indexesOfObjectsUsingBlock:(BOOL (^)(id object))block
{
    NSMutableIndexSet *indexes = [NSMutableIndexSet indexSet];
    NSUInteger i = 0;
    for (id object in self)
    {
        if (block(object))
        {
            [indexes addIndex:i];
        }
        i++;
    }
    return [indexes copy];
}

@end
