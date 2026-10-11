/*  HBAudioTrackPreset.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBAudioFilters;

typedef NS_ENUM(NSUInteger, HBAudioPresetEncoderMode)
{
    HBAudioPresetEncoderModeABR,
    HBAudioPresetEncoderModeQuality,
};

NS_ASSUME_NONNULL_BEGIN

/**
 *  HBAudioTrackPreset
 *  a KVO enabled class used in the Audio Defaults
 *  (Selection Behavior) panels, automatically
 *  validates the values.
 */
@interface HBAudioTrackPreset : NSObject <NSSecureCoding, NSCopying>

- (instancetype)initWithContainer:(int)container;
- (void)containerChanged:(int)container;

/**
 *  track properties.
 */
@property (nonatomic, readwrite) int encoder;
@property (nonatomic, readwrite) int fallbackEncoder;
@property (nonatomic, readwrite) int mixdown;
@property (nonatomic, readwrite) int sampleRate;

@property (nonatomic, readwrite) int bitRate;
@property (nonatomic, readwrite) double quality;
@property (nonatomic, readwrite) HBAudioPresetEncoderMode mode;

@property (nonatomic, readwrite) double gain;
@property (nonatomic, readwrite) double drc;

@property (nonatomic, readonly) HBAudioFilters *filters;

/**
 *  Arrays of possible options for the track properties.
 */
@property (nonatomic, readonly) NSArray<NSString *> *encoders;
@property (nonatomic, readonly) NSArray<NSString *> *mixdowns;
@property (nonatomic, readonly) NSArray<NSString *> *sampleRates;
@property (nonatomic, readonly) NSArray<NSNumber *> *bitRates;
@property (nonatomic, readonly) NSArray<NSNumber *> *qualities;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
