/*  HBSubtitles.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

NS_ASSUME_NONNULL_BEGIN

@class HBTitle;
@class HBSubtitlesTrack;
@class HBSubtitlesDefaults;

@interface HBSubtitles : NSObject <NSSecureCoding, NSCopying, HBPresetCoding>

- (instancetype)initWithTitle:(HBTitle *)title;

- (void)addAllTracks;
- (void)removeAll;
- (void)reloadDefaults;

- (void)addSrtTrackFromURL:(NSURL *)srtURL;

@property (nonatomic, readonly) NSMutableArray<NSDictionary *> *sourceTracks;
@property (nonatomic, readonly) NSMutableArray<HBSubtitlesTrack *> *tracks;

@property (nonatomic, readwrite, strong) HBSubtitlesDefaults *defaults;

/**
 *  For internal use
 */
@property (nonatomic, readwrite) int container;
@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

@interface HBSubtitles (KVC)

@property (nonatomic, readonly) NSUInteger countOfTracks;
- (HBSubtitlesTrack *)objectInTracksAtIndex:(NSUInteger)index;
- (void)insertObject:(HBSubtitlesTrack *)audioObject inTracksAtIndex:(NSUInteger)index;
- (void)removeObjectFromTracksAtIndex:(NSUInteger)index;

@end

NS_ASSUME_NONNULL_END

