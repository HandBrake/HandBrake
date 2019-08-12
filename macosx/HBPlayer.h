/*  HBPlayer.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */


#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

#import "HBPlayerTrack.h"

NS_ASSUME_NONNULL_BEGIN

/**
 *  A protocol for a generic player,
 *  users should call loadPlayableValueAsynchronouslyWithCompletionHandler
 *  before calling any other method or property.
 */
@protocol HBPlayer <NSObject>

- (instancetype)initWithURL:(NSURL *)url;

@property (nonatomic, readonly) CALayer *layer;

@property (nonatomic, readonly) NSTimeInterval duration;
@property (nonatomic) NSTimeInterval currentTime;

@property (nonatomic, readonly, copy) NSArray<HBPlayerTrack *> *audioTracks;
@property (nonatomic, readonly, copy) NSArray<HBPlayerTrack *> *subtitlesTracks;

- (void)enableAudioTrack:(nullable HBPlayerTrack *)playerTrack;
- (void)enableSubtitlesTrack:(nullable HBPlayerTrack *)playerTrack;

@property (nonatomic) float rate;
@property (nonatomic) float volume;

@property (nonatomic, readonly, getter=isPlayable) BOOL playable;

- (void)loadPlayableValueAsynchronouslyWithCompletionHandler:(nullable void (^)(void))handler;

- (id)addPeriodicTimeObserverUsingBlock:(void (^)(NSTimeInterval time))block;
- (void)removeTimeObserver:(id)observer;

- (id)addRateObserverUsingBlock:(void (^)(void))block;
- (void)removeRateObserver:(id)observer;

- (void)play;
- (void)pause;
- (void)gotoBeginning;
- (void)gotoEnd;
- (void)stepForward;
- (void)stepBackward;

@end

NS_ASSUME_NONNULL_END

