/*  HBSubtitlesSettings.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

typedef NS_ENUM(NSUInteger, HBSubtitleTrackSelectionBehavior) {
    HBSubtitleTrackSelectionBehaviorNone,
    HBSubtitleTrackSelectionBehaviorFirst,
    HBSubtitleTrackSelectionBehaviorAll,
};

@interface HBSubtitlesDefaults : NSObject <NSCoding, NSCopying, HBPresetCoding>

@property (nonatomic, readwrite) HBSubtitleTrackSelectionBehavior trackSelectionBehavior;
@property (nonatomic, readwrite, retain) NSMutableArray *trackSelectionLanguages;

@property (nonatomic, readwrite) BOOL addForeignAudioSearch;
@property (nonatomic, readwrite) BOOL addForeignAudioSubtitle;
@property (nonatomic, readwrite) BOOL addCC;

@end
