/*  HBVideo.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBPresetCoding.h>

@class HBJob;

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, HBVideoQualityType) {
    HBVideoQualityTypeAvgBitrate,
    HBVideoQualityTypeConstantQuality,
};

typedef NS_ENUM(NSUInteger, HBVideoFrameRateMode) {
    HBVideoFrameRateModeVFR_PFR,
    HBVideoFrameRateModeCFR,
};

extern NSString * const HBVideoChangedNotification;

/**
 *  HBVideo
 */
@interface HBVideo : NSObject <NSSecureCoding, NSCopying>

@property (nonatomic, readwrite) int encoder;

@property (nonatomic, readwrite) HBVideoQualityType qualityType;
@property (nonatomic, readwrite) int avgBitrate;
@property (nonatomic, readwrite) double quality;

@property (nonatomic, readwrite) HBVideoFrameRateMode frameRateMode;
@property (nonatomic, readwrite) int frameRate;

@property (nonatomic, readwrite) BOOL multiPass;
@property (nonatomic, readwrite) BOOL turboMultiPass;

/**
 *  Encoder specifics options
 */

@property (nonatomic, readwrite, copy) NSString *preset;
@property (nonatomic, readwrite, copy) NSString *tune;
@property (nonatomic, readwrite, copy) NSString *profile;
@property (nonatomic, readwrite, copy) NSString *level;

@property (nonatomic, readwrite, copy) NSString *videoOptionExtra;

@property (nonatomic, readwrite) BOOL fastDecode;

@property (nonatomic, readonly) NSString *completeTune;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
