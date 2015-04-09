/*  HBSubtitles.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

extern NSString *keySubTrackSelectionIndex;
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
extern NSString *keySubTrackSrtCharCodeIndex;
extern NSString *keySubTrackLanguageIndex;

@class HBTitle;
@class HBSubtitlesDefaults;

@interface HBSubtitles : NSObject <NSCoding, NSCopying, HBPresetCoding>

- (instancetype)initWithTitle:(HBTitle *)title;

- (void)addAllTracks;
- (void)removeAll;
- (void)reloadDefaults;

- (void)validatePassthru;
- (NSMutableDictionary *)createSubtitleTrack;
- (NSMutableDictionary *)trackFromSourceTrackIndex:(NSInteger)index;

@property (nonatomic, readonly) NSMutableArray *masterTrackArray;  // the master list of audio tracks from the title
@property (nonatomic, readonly) NSMutableArray *tracks;

@property (nonatomic, readwrite, strong) NSString *foreignAudioSearchTrackName;
@property (nonatomic, readonly) NSArray *charCodeArray;

@property (nonatomic, readonly) NSArray *languagesArray;
@property (nonatomic, readonly) NSInteger languagesArrayDefIndex;

@property (nonatomic, readwrite, strong) HBSubtitlesDefaults *defaults;

/**
 *  For internal use
 */

- (void)containerChanged:(int)container;
@property (nonatomic, readwrite) int container; // initially is the default HB_MUX_MP4

@end

@interface HBSubtitles (KVC)

@property (nonatomic, readonly) NSUInteger countOfTracks;
- (id)objectInTracksAtIndex:(NSUInteger)index;
- (void)insertObject:(id)audioObject inTracksAtIndex:(NSUInteger)index;
- (void)removeObjectFromTracksAtIndex:(NSUInteger)index;

@end

