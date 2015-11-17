/*  HBAudioSettings.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

NS_ASSUME_NONNULL_BEGIN

@class HBAudioTrackPreset;

typedef NS_ENUM(NSUInteger, HBAudioTrackSelectionBehavior) {
    HBAudioTrackSelectionBehaviorNone,
    HBAudioTrackSelectionBehaviorFirst,
    HBAudioTrackSelectionBehaviorAll,
};

/**
 *  HBAudioSettings
 *  Stores the audio defaults settings.
 */
@interface HBAudioDefaults : NSObject <NSSecureCoding, NSCopying, HBPresetCoding>

@property (nonatomic, readwrite) HBAudioTrackSelectionBehavior trackSelectionBehavior;
@property (nonatomic, readwrite, strong) NSMutableArray<NSString *> *trackSelectionLanguages;
@property (nonatomic, readwrite, strong) NSMutableArray<HBAudioTrackPreset *> *tracksArray;

/**
 *  Adds a new track preset.
 */
- (void)addTrack;

@property(nonatomic, readwrite) BOOL allowAACPassthru;
@property(nonatomic, readwrite) BOOL allowAC3Passthru;
@property(nonatomic, readwrite) BOOL allowEAC3Passthru;
@property(nonatomic, readwrite) BOOL allowDTSHDPassthru;
@property(nonatomic, readwrite) BOOL allowDTSPassthru;
@property(nonatomic, readwrite) BOOL allowMP3Passthru;
@property(nonatomic, readwrite) BOOL allowTrueHDPassthru;
@property(nonatomic, readwrite) BOOL allowFLACPassthru;

@property(nonatomic, readwrite) int encoderFallback;
@property(nonatomic, readwrite) BOOL secondaryEncoderMode;

@property(nonatomic, readonly) NSArray<NSString *> *audioEncoderFallbacks;

- (void)validateEncoderFallbackForVideoContainer:(int)container;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
