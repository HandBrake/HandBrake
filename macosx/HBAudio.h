/*  HBAudio.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

@class HBTitle;
@class HBAudioTrack;
@class HBAudioDefaults;

@interface HBAudio : NSObject <NSCoding, NSCopying, HBPresetCoding>

- (instancetype)initWithTitle:(HBTitle *)title;

@property (nonatomic, readonly) NSDictionary *noneTrack;
@property (nonatomic, readonly) NSArray *masterTrackArray;  // the master list of audio tracks from the title
@property (nonatomic, readonly) NSMutableArray *tracks;

@property (nonatomic, readwrite, retain) HBAudioDefaults *defaults;

- (void)addAllTracks;
- (void)removeAll;
- (void)reloadDefaults;

- (BOOL)anyCodecMatches:(int)codec;
- (void)settingTrackToNone:(HBAudioTrack *)newNoneTrack;
- (void)switchingTrackFromNone:(HBAudioTrack *)noLongerNoneTrack;

/**
 *  For internal use
 */

- (void)containerChanged:(int)container;

@end

@interface HBAudio (KVC)

- (NSUInteger)countOfTracks;
- (HBAudioTrack *)objectInTracksAtIndex:(NSUInteger)index;
- (void)insertObject:(HBAudioTrack *)track inTracksAtIndex:(NSUInteger)index;
- (void)removeObjectFromTracksAtIndex:(NSUInteger)index;

@end
