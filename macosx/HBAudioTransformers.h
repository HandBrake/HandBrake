/*  HBAudioTransformers.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

/**
 *  A series of value trasformers to bridge the libhb enums
 *  to the textual rapresentations used in the interface.
 */
@interface HBFallbackEncodersTransformer : NSValueTransformer
@end

@interface HBFallbackEncoderTransformer : NSValueTransformer
@end

@interface HBEncoderTransformer : NSValueTransformer
@end

@interface HBMixdownTransformer : NSValueTransformer
@end

@interface HBSampleRateTransformer : NSValueTransformer
@end

@interface HBIntegerTransformer : NSValueTransformer
@end
