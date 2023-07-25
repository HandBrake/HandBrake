/*  HBVideo+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBVideo.h>

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
@property (nonatomic, readonly) BOOL multiPassSupported;
@property (nonatomic, readonly) BOOL turboMultiPassSupported;

@property (nonatomic, readonly) NSString *unparseOptions;

@property (nonatomic, readonly) NSString *constantQualityLabel;

@property (nonatomic, readonly) double qualityMinValue;
@property (nonatomic, readonly) double qualityMaxValue;
@property (nonatomic, readonly) BOOL isConstantQualitySupported;

@end

/**
 *  A series of value transformers to bridge the libhb enums
 *  to the textual rapresentations used in the interface.
 */
@interface HBVideoEncoderTransformer : NSValueTransformer
@end

@interface HBFrameRateTransformer : NSValueTransformer
@end

@interface HBTuneTransformer : NSValueTransformer
@end

@interface HBTunesTransformer : NSValueTransformer
@end

@interface HBPresetsTransformer : NSValueTransformer
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithEncoder:(int)encoder NS_DESIGNATED_INITIALIZER;
@end

@interface HBQualityTransformer : NSValueTransformer
- (instancetype)initWithReversedDirection:(BOOL)reverse min:(double)min max:(double)max NS_DESIGNATED_INITIALIZER;
@end

@interface HBVideo (EncoderAdditions)

- (BOOL)isUnparsedSupported:(int)encoder;
- (BOOL)isPresetSystemSupported:(int)encoder;
- (BOOL)isSimpleOptionsPanelSupported:(int)encoder;
- (void)qualityLimitsForEncoder:(int)encoder low:(float *)low high:(float *)high granularity:(float *)granularity direction:(int *)direction;

@end
