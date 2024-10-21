/*  HBSubtitles.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBPresetCoding.h>
#import <HandBrakeKit/HBSecurityAccessToken.h>

NS_ASSUME_NONNULL_BEGIN

@class HBSubtitlesTrack;
@class HBTitleSubtitlesTrack;
@class HBSubtitlesDefaults;

@interface HBSubtitles : NSObject <NSSecureCoding, NSCopying, HBSecurityScope>

- (void)addAllTracks;
- (void)removeAll;
- (void)reloadDefaults;

- (void)addExternalSourceTrackFromURL:(NSURL *)fileURL addImmediately:(BOOL)addImmediately;

@property (nonatomic, readonly) NSArray<HBTitleSubtitlesTrack *> *sourceTracks;
@property (nonatomic, readonly) NSMutableArray<HBSubtitlesTrack *> *tracks;

@property (nonatomic, readwrite, strong) HBSubtitlesDefaults *defaults;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@property (nonatomic, readonly) NSUInteger countOfTracks;
- (HBSubtitlesTrack *)objectInTracksAtIndex:(NSUInteger)index;
- (void)insertObject:(HBSubtitlesTrack *)audioObject inTracksAtIndex:(NSUInteger)index;
- (void)removeObjectFromTracksAtIndex:(NSUInteger)index;

@end

NS_ASSUME_NONNULL_END

