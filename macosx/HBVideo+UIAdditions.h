/*  HBVideo+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBVideo.h"

@interface HBVideo (UIAdditions)

/**
 *  Arrays of possible options for the video properties.
 */
@property (nonatomic, readonly) NSArray *encoders;
@property (nonatomic, readonly) NSArray *frameRates;

@property (nonatomic, readonly) NSArray *presets;
@property (nonatomic, readonly) NSArray *tunes;
@property (nonatomic, readonly) NSArray *profiles;
@property (nonatomic, readonly) NSArray *levels;

@property (nonatomic, readonly) BOOL fastDecodeSupported;
@property (nonatomic, readonly) BOOL turboTwoPassSupported;

@property (nonatomic, readonly) NSString *unparseOptions;

@property (nonatomic, readonly) double qualityMinValue;
@property (nonatomic, readonly) double qualityMaxValue;

@end

/**
 *  A series of value trasformers to bridge the libhb enums
 *  to the textual rapresentations used in the interface.
 */
@interface HBVideoEncoderTransformer : NSValueTransformer
@end

@interface HBFrameRateTransformer : NSValueTransformer
@end

@interface HBPresetsTransformer : NSValueTransformer
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithEncoder:(int)encoder NS_DESIGNATED_INITIALIZER;
@end

@interface HBQualityTransformer : NSValueTransformer
- (instancetype)initWithReversedDirection:(BOOL)reverse min:(double)min max:(double)max NS_DESIGNATED_INITIALIZER;
@end
