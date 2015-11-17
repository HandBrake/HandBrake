/*  HBAudioTrackPreset.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 *  HBAudioTrackPreset
 *  a KVO enabled class used in the Audio Defaults panels,
 *  automatically validates the values.
 */
@interface HBAudioTrackPreset : NSObject <NSSecureCoding, NSCopying>

- (instancetype)initWithContainer:(int)container;
- (void)containerChanged:(int)container;

/**
 *  track properties.
 */
@property (nonatomic, readwrite) int encoder;
@property (nonatomic, readwrite) int mixdown;
@property (nonatomic, readwrite) int sampleRate;
@property (nonatomic, readwrite) int bitRate;

@property (nonatomic, readwrite) double gain;
@property (nonatomic, readwrite) double drc;

/**
 *  Arrays of possible options for the track properties.
 */
@property (nonatomic, readonly) NSArray<NSString *> *encoders;
@property (nonatomic, readonly) NSArray<NSString *> *mixdowns;
@property (nonatomic, readonly) NSArray<NSString *> *sampleRates;
@property (nonatomic, readonly) NSArray<NSString *> *bitRates;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END

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