/* $Id: HBSubtitles.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBViewValidation.h"

extern NSString *keySubTrackName;
extern NSString *keySubTrackIndex;
extern NSString *keySubTrackLanguage;
extern NSString *keySubTrackLanguageIsoCode;
extern NSString *keySubTrackType;

extern NSString *keySubTrackForced;
extern NSString *keySubTrackBurned;
extern NSString *keySubTrackDefault;

extern NSString *keySubTrackSrtOffset;
extern NSString *keySubTrackSrtFilePath;
extern NSString *keySubTrackSrtCharCode;

@class HBJob;

/**
 *  HBSubtitlesController
 *  Responds to HBContainerChangedNotification.
 */
@interface HBSubtitlesController : NSViewController <HBViewValidation>

- (void)addTracksFromQueue:(NSArray *)queueSubtitleArray;
- (void)applySettingsFromPreset:(NSDictionary *)preset;

@property (nonatomic, readwrite, assign) HBJob *job;

// Get the list of subtitles tracks
@property (readonly, nonatomic, copy) NSArray *subtitles;

@end
