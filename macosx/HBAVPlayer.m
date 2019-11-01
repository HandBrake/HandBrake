/*  HBAVPlayer.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAVPlayer.h"

@import AVFoundation;
@import HandBrakeKit;

static void *HBAVPlayerRateContext = &HBAVPlayerRateContext;
static void *HBAVPlayerItemStatusContext = &HBAVPlayerItemStatusContext;

typedef void (^HBRateObverser)(void);
typedef void (^HBPlayableObverser)(void);

@interface HBAVPlayerRateObserver : NSObject

@property (nonatomic, copy) HBRateObverser block;

- (void)postNotification;

@end

@implementation HBAVPlayerRateObserver

- (void)postNotification
{
    self.block();
}

@end

@interface HBAVPlayer ()

@property (nonatomic, strong) AVAsset *movie;
@property (nonatomic, strong) AVPlayer *player;

@property (nonatomic, strong) NSMutableSet<HBAVPlayerRateObserver *> *rateObservers;
@property (nonatomic, strong) NSMutableSet<HBPlayableObverser> *playableObservers;

@property (nonatomic, readwrite, getter=isPlayable) BOOL playable;
@property (nonatomic, readwrite, getter=isLoaded) BOOL loaded;

@end

@implementation HBAVPlayer

- (instancetype)initWithURL:(NSURL *)url
{
    self = [super init];

    if (self)
    {
        _movie = [AVAsset assetWithURL:url];;
        _player = [[AVPlayer alloc] init];
        _layer = [CALayer layer];

        _rateObservers = [NSMutableSet set];
        _playableObservers = [NSMutableSet set];

        [self addObserver:self forKeyPath:@"player.rate" options:NSKeyValueObservingOptionNew context:HBAVPlayerRateContext];
        [self addObserver:self forKeyPath:@"player.currentItem.status" options:NSKeyValueObservingOptionNew context:HBAVPlayerItemStatusContext];

        NSArray *assetKeysToLoadAndTest = @[@"playable"];
        [_movie loadValuesAsynchronouslyForKeys:assetKeysToLoadAndTest completionHandler:^(void) {

            // The asset invokes its completion handler on an arbitrary queue when loading is complete.
            // Because we want to access our AVPlayer in our ensuing set-up, we must dispatch our handler to the main queue.
            dispatch_async(dispatch_get_main_queue(), ^(void) {
                [self _setUpPlaybackOfAsset:self->_movie withKeys:assetKeysToLoadAndTest];
            });

        }];
    }

    return self;
}

- (void)dealloc
{
    @try
    {
        [self removeObserver:self forKeyPath:@"player.rate"];
        [self removeObserver:self forKeyPath:@"player.currentItem.status"];
    }
    @catch (NSException *exception) {}
}

- (void)_setUpPlaybackOfAsset:(AVAsset *)asset withKeys:(NSArray *)keys
{
    // First test whether the values of each of the keys we need have been successfully loaded.
    for (NSString *key in keys)
    {
        NSError *error = nil;

        if ([asset statusOfValueForKey:key error:&error] == AVKeyValueStatusFailed)
        {
            self.playable = NO;
            return;
        }

        if (!asset.isPlayable)
        {
            self.playable = NO;
            return;
        }

        AVPlayerItem *playerItem = [AVPlayerItem playerItemWithAsset:asset];
        [self.player replaceCurrentItemWithPlayerItem:playerItem];

        AVPlayerLayer *playerLayer = [AVPlayerLayer playerLayerWithPlayer:_player];
        playerLayer.frame = self.layer.bounds;
        playerLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;

        [self.layer addSublayer:playerLayer];
    }
}

- (void)setLoaded:(BOOL)loaded
{
    _loaded = loaded;

    for (HBPlayableObverser block in self.playableObservers)
    {
        block();
    }
    [self.playableObservers removeAllObjects];
}

- (void)setPlayable:(BOOL)playable
{
    _playable = playable;
    self.loaded = YES;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBAVPlayerItemStatusContext)
    {
        AVPlayerStatus status = [change[NSKeyValueChangeNewKey] integerValue];
        switch (status)
        {
            case AVPlayerItemStatusUnknown:
                break;
            case AVPlayerItemStatusReadyToPlay:
                self.playable = YES;
                break;
            case AVPlayerItemStatusFailed:
                self.playable = NO;
                break;
        }

    }
    else if (context == HBAVPlayerRateContext)
    {
        for (HBAVPlayerRateObserver *observer in self.rateObservers)
        {
            [observer postNotification];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

#pragma mark Public properties

- (NSArray<HBPlayerTrack *> *)tracksWithMediaType:(NSString *)mediaType
{
    NSMutableArray *result = [NSMutableArray array];
    NSArray<AVPlayerItemTrack *> *tracks = self.player.currentItem.tracks;

    for (AVPlayerItemTrack *track in tracks)
    {
        AVAssetTrack *assetTrack = track.assetTrack;
        if ([assetTrack.mediaType isEqualToString:mediaType])
        {
            NSString *name = [HBUtilities languageCodeForIso6392Code:assetTrack.languageCode];
            HBPlayerTrack *playerTrack = [[HBPlayerTrack alloc] initWithTrackName:name object:@(assetTrack.trackID) enabled:track.isEnabled];
            [result addObject:playerTrack];
        }
    }
    return result;
}

@synthesize audioTracks = _audioTracks;

- (NSArray<HBPlayerTrack *> *)audioTracks
{
    if (_audioTracks == nil)
    {
        _audioTracks = [self tracksWithMediaType:AVMediaTypeAudio];
    }
    return _audioTracks;
}

@synthesize subtitlesTracks = _subtitlesTracks;

- (NSArray<HBPlayerTrack *> *)subtitlesTracks
{
    if (_subtitlesTracks == nil)
    {
        _subtitlesTracks = [self tracksWithMediaType:AVMediaTypeSubtitle];
    }
    return _subtitlesTracks;
}

- (void)_enableTrack:(HBPlayerTrack *)playerTrack mediaType:(NSString *)mediaType
{
    NSArray<AVPlayerItemTrack *> *tracks = self.player.currentItem.tracks;
    for (AVPlayerItemTrack *track in tracks)
    {
        if ([track.assetTrack.mediaType isEqualToString:mediaType])
        {
            if (track.assetTrack.trackID == [playerTrack.representedObject integerValue])
            {
                track.enabled = YES;
            }
            else
            {
                track.enabled = NO;
            }
        }
    }
}

- (void)enableAudioTrack:(HBPlayerTrack *)playerTrack
{
    for (HBPlayerTrack *track in self.audioTracks)
    {
        track.enabled = NO;
    }
    playerTrack.enabled = YES;
    [self _enableTrack:playerTrack mediaType:AVMediaTypeAudio];
}

- (void)enableSubtitlesTrack:(HBPlayerTrack *)playerTrack
{
    for (HBPlayerTrack *track in self.subtitlesTracks)
    {
        track.enabled = NO;
    }
    playerTrack.enabled = YES;
    [self _enableTrack:playerTrack mediaType:AVMediaTypeSubtitle];
}

- (NSTimeInterval)duration
{
    return CMTimeGetSeconds(self.movie.duration);
}

- (NSTimeInterval)currentTime
{
    return CMTimeGetSeconds(self.player.currentTime);
}

- (void)setCurrentTime:(NSTimeInterval)value
{
    [self.player seekToTime:CMTimeMake((int64_t)(value * 1000), 1000) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
}

- (void)setRate:(float)rate
{
    self.player.rate = rate;
}

- (float)rate
{
    return self.player.rate;
}

- (float)volume
{
    return self.player.volume;
}

- (void)setVolume:(float)volume
{
    self.player.volume = volume;
}

#pragma mark public methods

- (void)loadPlayableValueAsynchronouslyWithCompletionHandler:(nullable void (^)(void))handler
{
    if (self.isLoaded)
    {
        handler();
    }
    else
    {
        [self.playableObservers addObject:handler];
    }
}

- (id)addPeriodicTimeObserverUsingBlock:(void (^)(NSTimeInterval time))block
{
    CMTime interval = CMTimeMake(100, 1000);

    id observer = [self.player addPeriodicTimeObserverForInterval:interval queue:dispatch_get_main_queue() usingBlock:^(CMTime time) {
        block(0);
    }];

    return observer;
}

- (void)removeTimeObserver:(id)observer
{
    [self.player removeTimeObserver:observer];
}

- (id)addRateObserverUsingBlock:(void (^)(void))block
{
    HBAVPlayerRateObserver *observer = [[HBAVPlayerRateObserver alloc] init];
    observer.block = block;
    [self.rateObservers addObject:observer];

    return observer;
}

- (void)removeRateObserver:(id)observer
{
    [self.rateObservers removeObject:observer];
}

- (void)play
{
    [self.player play];
}

- (void)pause
{
    [self.player pause];
}

- (void)gotoBeginning
{
    [self.player seekToTime:kCMTimeZero toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
}

- (void)gotoEnd
{
    [self.player seekToTime:self.player.currentItem.duration toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
}

- (void)stepForward
{
    CMTime frameTime = CMTimeMakeWithSeconds(1.0/30.0, self.player.currentItem.duration.timescale);
    CMTime forwardTime = CMTimeAdd([self.player.currentItem currentTime], frameTime);
    [self.player seekToTime:forwardTime  toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
}

- (void)stepBackward
{
    CMTime frameTime = CMTimeMakeWithSeconds(-1.0/30.0, self.player.currentItem.duration.timescale);
    CMTime backwardTime = CMTimeAdd([self.player.currentItem currentTime], frameTime);
    [self.player seekToTime:backwardTime toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
}

@end
