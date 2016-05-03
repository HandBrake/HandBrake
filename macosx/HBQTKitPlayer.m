/*  HBQTKitPlayer.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQTKitPlayer.h"
#import <QTKit/QTKit.h>

@import HandBrakeKit;

@interface QTTrack (HBAdditions)
- (id)isoLanguageCodeAsString;
@end

typedef void (^HBPeriodicObverser)(NSTimeInterval time);
typedef void (^HBRateObverser)(void);
typedef void (^HBPlayableObverser)(void);

@interface HBQTKitPlayerPeriodicObserver : NSObject

@property (nonatomic) HBPeriodicObverser block;

- (void)postNotification:(NSTimeInterval)time;

@end

@implementation HBQTKitPlayerPeriodicObserver

- (void)postNotification:(NSTimeInterval)time
{
    self.block(time);
}

@end

@interface HBQTKitPlayerRateObserver : NSObject

@property (nonatomic) HBRateObverser block;

- (void)postNotification;

@end

@implementation HBQTKitPlayerRateObserver

- (void)postNotification;
{
    self.block();
}

@end

@interface HBQTKitPlayer ()

@property (nonatomic, strong) QTMovie *movie;
@property (nonatomic, strong) NSTimer *timer;

@property (nonatomic, readwrite, getter=isPlayable) BOOL playable;

@property (nonatomic, strong) NSMutableSet<HBQTKitPlayerPeriodicObserver *> *periodicObservers;
@property (nonatomic, strong) NSMutableSet<HBQTKitPlayerRateObserver *> *rateObservers;

@property (nonatomic, strong) NSMutableSet<HBPlayableObverser> *playableObservers;

@end

@implementation HBQTKitPlayer

- (instancetype)initWithURL:(NSURL *)url
{
    self = [super init];

    if (self)
    {
        NSError *outError;
        NSDictionary *attributes = @{ QTMovieURLAttribute: url,
                                      QTMovieAskUnresolvedDataRefsAttribute: @NO,
                                      QTMovieOpenForPlaybackAttribute: @YES,
                                      QTMovieIsSteppableAttribute: @YES,
                                      QTMovieOpenAsyncRequiredAttribute: @YES,
                                      QTMovieApertureModeAttribute: QTMovieApertureModeClean };

        _movie = [[QTMovie alloc] initWithAttributes:attributes error:&outError];
        
        if (!_movie)
        {
            return nil;
        }

        _movie.delegate = self;

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(_movieRateDidChange:)
                                                     name:QTMovieRateDidChangeNotification
                                                   object:_movie];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(_loadStateChanged:)
                                                     name:QTMovieLoadStateDidChangeNotification
                                                   object:_movie];

        _layer = [QTMovieLayer layerWithMovie:_movie];

        if (!_layer)
        {
            return nil;
        }

        _periodicObservers = [NSMutableSet set];
        _rateObservers = [NSMutableSet set];
        _playableObservers = [NSMutableSet set];
    }

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self _stopMovieTimer];
}

- (void)setPlayable:(BOOL)playable
{
    _playable = playable;

    for (HBPlayableObverser block in self.playableObservers)
    {
        block();
    }
    [self.playableObservers removeAllObjects];
}

- (void)_loadStateChanged:(NSNotification *)notification
{
    int loadState = [[self.movie attributeForKey:QTMovieLoadStateAttribute] intValue];

    if (loadState >= QTMovieLoadStateLoaded)
    {
        [self _enableSubtitles];
        self.playable = YES;
    }
}

- (void)_movieRateDidChange:(NSNotification *)notification
{
    for (HBQTKitPlayerRateObserver *observer in self.rateObservers)
    {
        [observer postNotification];
    }
}

- (void)_enableSubtitles
{
    // Get and enable subtitles
    NSArray<QTTrack *> *subtitlesArray = [self.movie tracksOfMediaType:QTMediaTypeSubtitle];
    if (subtitlesArray.count)
    {
        // enable the first tx3g subtitle track
        [subtitlesArray.firstObject setEnabled:YES];
    }
    else
    {
        // Perian subtitles
        subtitlesArray = [self.movie tracksOfMediaType:QTMediaTypeVideo];
        if (subtitlesArray.count >= 2)
        {
            // track 0 should be video, other video tracks should
            // be subtitles; force-enable the first subs track
            [subtitlesArray[1] setEnabled:YES];
        }
    }
}

- (void)_startMovieTimer
{
    if (!self.timer)
    {
        self.timer = [NSTimer scheduledTimerWithTimeInterval:0.09 target:self
                                                         selector:@selector(_timerFired:)
                                                         userInfo:nil repeats:YES];
    }
}

- (void)_stopMovieTimer
{
    [self.timer invalidate];
    self.timer = nil;
}

- (void)_timerFired:(NSTimer *)timer
{
    for (HBQTKitPlayerPeriodicObserver *observer in self.periodicObservers)
    {
        [observer postNotification:self.currentTime];
    }
}

#pragma mark Public properties

- (NSArray<HBPlayerTrack *> *)tracksWithMediaType:(NSString *)mediaType
{
    NSMutableArray *result = [NSMutableArray array];
    NSArray<QTTrack *> *tracks = [self.movie tracksOfMediaType:mediaType];
    for (QTTrack *track in tracks)
    {
        NSNumber *trackID = [track attributeForKey:QTTrackIDAttribute];
        NSString *name = NSLocalizedString(@"Unknown", nil);

        if ([track respondsToSelector:@selector(isoLanguageCodeAsString)])
        {
            NSString *language = [track isoLanguageCodeAsString];
            name = [HBUtilities languageCodeForIso6392Code:language];
        }
        BOOL enabled = [[track attributeForKey:QTTrackEnabledAttribute] boolValue];

        HBPlayerTrack *playerTrack = [[HBPlayerTrack alloc] initWithTrackName:name object:trackID enabled:enabled];

        [result addObject:playerTrack];
    }
    return result;
}

@synthesize audioTracks = _audioTracks;

- (NSArray<HBPlayerTrack *> *)audioTracks
{
    if (_audioTracks == nil)
    {
        _audioTracks = [self tracksWithMediaType:QTMediaTypeSound];
    }
    return _audioTracks;
}

@synthesize subtitlesTracks = _subtitlesTracks;

- (NSArray<HBPlayerTrack *> *)subtitlesTracks
{
    if (_subtitlesTracks == nil)
    {
        _subtitlesTracks = [self tracksWithMediaType:QTMediaTypeSubtitle];
    }
    return _subtitlesTracks;
}

- (void)_enableTrack:(HBPlayerTrack *)playerTrack mediaType:(NSString *)mediaType
{
    NSArray<QTTrack *> *tracks = [self.movie tracksOfMediaType:mediaType];
    for (QTTrack *track in tracks)
    {
        NSNumber *trackID = [track attributeForKey:QTTrackIDAttribute];

        if ([trackID isEqualTo:playerTrack.representedObject])
        {
            [track setEnabled:YES];
        }
        else
        {
            [track setEnabled:NO];
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
    [self _enableTrack:playerTrack mediaType:QTMediaTypeSound];
}

- (void)enableSubtitlesTrack:(HBPlayerTrack *)playerTrack
{
    for (HBPlayerTrack *track in self.subtitlesTracks)
    {
        track.enabled = NO;
    }
    playerTrack.enabled = YES;
    [self _enableTrack:playerTrack mediaType:QTMediaTypeSubtitle];
}

- (NSTimeInterval)duration
{
    QTTime duration = [self.movie duration];
    return (double)duration.timeValue / (double)duration.timeScale;;
}

- (NSTimeInterval)currentTime
{
    QTTime time = [self.movie currentTime];
    return (double)time.timeValue / (double)time.timeScale;;
}

- (void)setCurrentTime:(NSTimeInterval)value
{
    long timeScale = [[self.movie attributeForKey:QTMovieTimeScaleAttribute] longValue];
    [self.movie setCurrentTime:QTMakeTime((long long)value * timeScale, timeScale)];
    [self _timerFired:nil];
}

- (void)setRate:(float)rate
{
    self.movie.rate = rate;
}

- (float)rate
{
    return self.movie.rate;
}

- (float)volume
{
    return self.movie.volume;
}

- (void)setVolume:(float)volume
{
    self.movie.volume = volume;
}

#pragma mark public methods

- (void)loadPlayableValueAsynchronouslyWithCompletionHandler:(nullable void (^)(void))handler;
{
    if (self.playable)
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
    HBQTKitPlayerPeriodicObserver *observer = [[HBQTKitPlayerPeriodicObserver alloc] init];
    observer.block = block;
    [self.periodicObservers addObject:observer];

    [self _startMovieTimer];

    return observer;
}

- (void)removeTimeObserver:(id)observer
{
    [self.periodicObservers removeObject:observer];
}

- (id)addRateObserverUsingBlock:(void (^)(void))block
{
    HBQTKitPlayerRateObserver *observer = [[HBQTKitPlayerRateObserver alloc] init];
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
    [self.movie play];
    [self _startMovieTimer];
}

- (void)pause
{
    [self.movie stop];
    [self _stopMovieTimer];
}

- (void)gotoBeginning
{
    [self.movie gotoBeginning];
    [self _timerFired:nil];
}

- (void)gotoEnd
{
    [self.movie gotoEnd];
    [self _timerFired:nil];
}

- (void)stepForward
{
    [self.movie stepForward];
    [self _timerFired:nil];
}

- (void)stepBackward
{
    [self.movie stepBackward];
    [self _timerFired:nil];
}

@end
