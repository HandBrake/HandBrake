/*  HBSubtitles.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

NS_ASSUME_NONNULL_BEGIN

@class HBSubtitlesTrack;
@class HBSubtitlesDefaults;

@interface HBSubtitles : NSObject <NSSecureCoding, NSCopying>

- (void)addAllTracks;
- (void)removeAll;
- (void)reloadDefaults;

- (void)addSrtTrackFromURL:(NSURL *)srtURL;

@property (nonatomic, readonly) NSArray<NSDictionary *> *sourceTracks;
@property (nonatomic, readonly) NSMutableArray<HBSubtitlesTrack *> *tracks;

@property (nonatomic, readwrite, strong) HBSubtitlesDefaults *defaults;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

@interface HBSubtitles (KVC)

@property (nonatomic, readonly) NSUInteger countOfTracks;
- (HBSubtitlesTrack *)objectInTracksAtIndex:(NSUInteger)index;
- (void)insertObject:(HBSubtitlesTrack *)audioObject inTracksAtIndex:(NSUInteger)index;
- (void)removeObjectFromTracksAtIndex:(NSUInteger)index;

@end

NS_ASSUME_NONNULL_END

