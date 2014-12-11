/*  HBVideo.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#include "hb.h"

@interface HBVideo : NSObject

- (void)applySettingsFromPreset:(NSDictionary *)preset;
- (void)prepareVideoForPreset:(NSMutableDictionary *)preset;

- (void)applyVideoSettingsFromQueue:(NSDictionary *)queueToApply;
- (void)prepareVideoForQueueFileJob:(NSMutableDictionary *)queueFileJob;

- (void)prepareVideoForJobPreview:(hb_job_t *)job andTitle:(hb_title_t *)title;

@property (nonatomic, readwrite) int encoder;

@property (nonatomic, readwrite) int qualityType;
@property (nonatomic, readwrite) int avgBitrate;
@property (nonatomic, readwrite) double quality;

@property (nonatomic, readwrite) int frameRate;
@property (nonatomic, readwrite) int frameRateMode;

@property (nonatomic, readwrite) BOOL fastFirstPass;
@property (nonatomic, readwrite) BOOL twoPass;
@property (nonatomic, readwrite) BOOL turboTwoPass;

/**
 *  Encoder specifics options
 */

@property (nonatomic, readwrite) BOOL advancedOptions;
@property (nonatomic, readwrite, copy) NSString *preset;
@property (nonatomic, readwrite, copy) NSString *tune;
@property (nonatomic, readwrite, copy) NSString *profile;
@property (nonatomic, readwrite, copy) NSString *level;

@property (nonatomic, readwrite, copy) NSString *videoOptionExtra;

@property (nonatomic, readwrite) BOOL fastDecode;

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

@property (nonatomic, readonly) NSString *unparseOptions;

@property (nonatomic, readonly) double qualityMinValue;
@property (nonatomic, readonly) double qualityMaxValue;

/**
 * Width and height for x264 unparse. Will be removed later.
 */
@property (nonatomic, readwrite) int widthForUnparse;
@property (nonatomic, readwrite) int heightForUnparse;

/**
 *  Current container, this will be removed later too.
 */
@property (nonatomic, readwrite) int container;

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
- (instancetype)initWithEncoder:(int)encoder;
@end

@interface HBQualityTransformer : NSValueTransformer
- (instancetype)initWithReversedDirection:(BOOL)reverse min:(double)min max:(double)max;
@end
