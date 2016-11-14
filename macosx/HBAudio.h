/*  HBAudio.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

@class HBAudioTrack;
@class HBAudioDefaults;

NS_ASSUME_NONNULL_BEGIN

extern NSString *HBAudioChangedNotification;

@interface HBAudio : NSObject <NSSecureCoding, NSCopying>

@property (nonatomic, readonly) NSArray<NSDictionary *> *sourceTracks;
@property (nonatomic, readonly) NSMutableArray<HBAudioTrack *> *tracks;

@property (nonatomic, readwrite) HBAudioDefaults *defaults;

- (void)addAllTracks;
- (void)removeAll;
- (void)reloadDefaults;

- (BOOL)anyCodecMatches:(int)codec;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

@interface HBAudio (KVC)

@property (nonatomic, readonly) NSUInteger countOfTracks;
- (HBAudioTrack *)objectInTracksAtIndex:(NSUInteger)index;
- (void)insertObject:(HBAudioTrack *)track inTracksAtIndex:(NSUInteger)index;
- (void)removeObjectFromTracksAtIndex:(NSUInteger)index;

@end

NS_ASSUME_NONNULL_END
