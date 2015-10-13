//
//  HBMutablePreset.h
//  HandBrake
//
//  Created by Damiano Galassi on 12/10/15.
//
//

#import <Foundation/Foundation.h>
#import "HBPreset.h"

NS_ASSUME_NONNULL_BEGIN

/**
 *  A mutable subclass of HBPreset.
 */
@interface HBMutablePreset : HBPreset

/**
 *  Removes unknown keys and normalizes values.
 */
- (void)cleanUp;

- (void)setObject:(id)obj forKey:(NSString *)key;
- (void)setObject:(id)obj forKeyedSubscript:(NSString *)key;

@end

NS_ASSUME_NONNULL_END
