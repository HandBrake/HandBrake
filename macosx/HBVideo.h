/*  HBVideo.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

@class HBJob;

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HBVideoChangedNotification;

/**
 *  HBVideo
 */
@interface HBVideo : NSObject <NSSecureCoding, NSCopying, HBPresetCoding>

- (instancetype)initWithJob:(HBJob *)job;

- (void)containerChanged;

@property (nonatomic, readwrite) int encoder;

@property (nonatomic, readwrite) int qualityType;
@property (nonatomic, readwrite) int avgBitrate;
@property (nonatomic, readwrite) double quality;

@property (nonatomic, readwrite) int frameRate;
@property (nonatomic, readwrite) int frameRateMode;

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

@property (nonatomic, readwrite, weak) HBJob *job;
@property (nonatomic, readonly) NSString *completeTune;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
