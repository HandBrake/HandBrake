//
//  NSArray+NSArray_HBArrayAdditions.h
//  HandBrake
//
//  Created by Damiano Galassi on 22/07/15.
//
//

#import <Foundation/Foundation.h>

@interface NSArray (HBAdditions)

- (NSArray *)filteredArrayUsingBlock:(BOOL (^)(id object))block;
- (NSIndexSet *)indexesOfObjectsUsingBlock:(BOOL (^)(id object))block;

@end