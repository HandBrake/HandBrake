/*  HBSubtitlesSettings.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBPresetCoding.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, HBSubtitleTrackSelectionBehavior) {
    HBSubtitleTrackSelectionBehaviorNone,
    HBSubtitleTrackSelectionBehaviorFirst,
    HBSubtitleTrackSelectionBehaviorAll,
};

typedef NS_ENUM(NSUInteger, HBSubtitleTrackBurnInBehavior) {
    HBSubtitleTrackBurnInBehaviorNone,
    HBSubtitleTrackBurnInBehaviorForeignAudio,
    HBSubtitleTrackBurnInBehaviorFirst,
    HBSubtitleTrackBurnInBehaviorForeignAudioThenFirst,
};

@interface HBSubtitlesDefaults : NSObject <NSSecureCoding, NSCopying, HBPresetCoding>

@property (nonatomic, readwrite) HBSubtitleTrackSelectionBehavior trackSelectionBehavior;
@property (nonatomic, readwrite, strong) NSMutableArray<NSString *> *trackSelectionLanguages;

@property (nonatomic, readwrite) BOOL addForeignAudioSearch;
@property (nonatomic, readwrite) BOOL addForeignAudioSubtitle;
@property (nonatomic, readwrite) BOOL addCC;

@property (nonatomic, readwrite) HBSubtitleTrackBurnInBehavior burnInBehavior;
@property (nonatomic, readwrite) BOOL burnInDVDSubtitles;
@property (nonatomic, readwrite) BOOL burnInBluraySubtitles;

@property (nonatomic, readwrite) BOOL passthruName;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
