//
//  NSArray+NSArray_HBArrayAdditions.m
//  HandBrake
//
//  Created by Damiano Galassi on 22/07/15.
//
//

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
