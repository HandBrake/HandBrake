/*  HBSubtitlesSettings.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, HBSubtitleTrackSelectionBehavior) {
    HBSubtitleTrackSelectionBehaviorNone,
    HBSubtitleTrackSelectionBehaviorFirst,
    HBSubtitleTrackSelectionBehaviorAll,
};

@interface HBSubtitlesDefaults : NSObject

@property (nonatomic, readwrite) HBSubtitleTrackSelectionBehavior trackSelectionBehavior;
@property (nonatomic, readwrite, retain) NSMutableArray *trackSelectionLanguages;

@property (nonatomic, readwrite) BOOL addForeignAudioSearch;
@property (nonatomic, readwrite) BOOL addForeignAudioSubtitle;
@property (nonatomic, readwrite) BOOL addCC;

- (void)applySettingsFromPreset:(NSDictionary *)preset;
- (void)prepareSubtitlesForPreset:(NSMutableDictionary *)preset;

@end
