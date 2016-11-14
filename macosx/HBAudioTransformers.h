//
//  HBAudioTransformers.h
//  HandBrake
//
//  Created by Damiano Galassi on 26/08/2016.
//
//

#import <Foundation/Foundation.h>

/**
 *  A series of value trasformers to bridge the libhb enums
 *  to the textual rapresentations used in the interface.
 */
@interface HBEncoderTransformer : NSValueTransformer
@end

@interface HBMixdownTransformer : NSValueTransformer
@end

@interface HBSampleRateTransformer : NSValueTransformer
@end

@interface HBIntegerTransformer : NSValueTransformer
@end
