/* $Id: HBSubtitles.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

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

/**
 *  HBSubtitlesController
 *  Responds to HBContainerChangedNotification and HBTitleChangedNotification notifications.
 */
@interface HBSubtitlesController : NSViewController

- (void)enableUI:(BOOL)b;
- (void)addTracksFromQueue:(NSMutableArray *)newSubtitleArray;

- (void)applySettingsFromPreset:(NSDictionary *)preset;
- (void)prepareSubtitlesForPreset:(NSMutableDictionary *)preset;

// Get the list of subtitles tracks
@property (readonly, nonatomic, copy) NSArray *subtitles;

@end
