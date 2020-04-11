/*  NSArray+HBAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "NSArray+HBAdditions.h"

@implementation NSArray (HBAdditions)

- (BOOL)allSatisfy:(BOOL (^)(id object))block
{
    for (id object in self)
    {
        if (block(object) == NO)
        {
            return NO;
        }
    }
    return YES;
}

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

@implementation NSIndexSet (HBAdditions)

- (NSIndexSet *)unionWith:(NSIndexSet *)indexSet
{
    NSMutableIndexSet *result = [self mutableCopy];
    [result addIndexes:indexSet];
    return result;
}

- (NSIndexSet *)intersectionWith:(NSIndexSet *)indexSet
{
    NSMutableIndexSet *result = [self mutableCopy];
    [result addIndexes:indexSet];
    [result removeIndexes:[self symmetricDifferenceWith:indexSet]];
    return result;
}

- (NSIndexSet *)relativeComplementIn:(NSIndexSet *)universe
{
    NSMutableIndexSet *complement = [universe mutableCopy];
    [complement removeIndexes:self];
    return complement;
}

- (NSIndexSet *)symmetricDifferenceWith:(NSIndexSet *)indexSet
{
    NSMutableIndexSet *a = [self mutableCopy];
    NSMutableIndexSet *b = [indexSet mutableCopy];
    [a removeIndexes:indexSet];
    [b removeIndexes:self];
    [a addIndexes:b];
    return a;
}

@end
