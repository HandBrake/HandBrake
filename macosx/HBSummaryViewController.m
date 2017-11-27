/*  HBSummaryViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSummaryViewController.h"
#import "HBPreviewView.h"
#import "HBPreviewGenerator.h"

@import HandBrakeKit;

static void *HBSummaryViewControllerContext = &HBSummaryViewControllerContext;

@interface HBSummaryViewController ()

@property (nonatomic, strong) IBOutlet NSLayoutConstraint *bottomOptionsConstrain;

@property (nonatomic, strong) IBOutlet NSTextField *tracksLabel;
@property (nonatomic, strong) IBOutlet NSTextField *filtersLabel;
@property (nonatomic, strong) IBOutlet NSTextField *dimensionLabel;

@property (nonatomic, strong) IBOutlet HBPreviewView *previewView;

@property (nonatomic) BOOL tracksReloadInQueue;
@property (nonatomic) BOOL filtersReloadInQueue;
@property (nonatomic) BOOL pictureReloadInQueue;

@property (nonatomic) BOOL visible;

@end

@implementation HBSummaryViewController

- (instancetype)init
{
    self = [super initWithNibName:@"HBSummaryViewController" bundle:nil];
    return self;
}

- (void)loadView
{
    [super loadView];
    self.previewView.showShadow = NO;
    self.visible = YES;
    [self resetLabels];
}

- (void)viewWillAppear
{
    self.visible = YES;
    if (self.pictureReloadInQueue || self.previewView.image == NULL)
    {
        [self updatePicture];
    }
}

- (void)viewDidDisappear
{
    self.visible = NO;
}

- (void)setGenerator:(HBPreviewGenerator *)generator
{
    _generator = generator;

    if (generator)
    {
        [self updatePicture];
    }
    else
    {
        self.previewView.image = nil;
    }
}

- (void)setJob:(HBJob *)job
{
    if (job)
    {
        [self removeJobObservers];
        _job = job;
        [self addJobObservers];
        [self updateTracksLabel];
        [self updateFiltersLabel];
    }
    else
    {
        [self removeJobObservers];
        [self resetLabels];
        _job = job;
    }
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBSummaryViewControllerContext)
    {
        if ([keyPath isEqualToString:@"audio.tracks"])
        {
            if ([change[NSKeyValueChangeKindKey] integerValue] == NSKeyValueChangeInsertion)
            {
                [self addAudioTracksObservers:change[NSKeyValueChangeNewKey]];
            }
            else if ([change[NSKeyValueChangeKindKey] integerValue]== NSKeyValueChangeRemoval)
            {
                [self removeAudioTracksObservers:change[NSKeyValueChangeOldKey]];
            }
        }
        else if ([keyPath isEqualToString:@"subtitles.tracks"])
        {
            if ([change[NSKeyValueChangeKindKey] integerValue] == NSKeyValueChangeInsertion)
            {
                [self addSubtitlesTracksObservers:change[NSKeyValueChangeNewKey]];
            }
            else if ([change[NSKeyValueChangeKindKey] integerValue]== NSKeyValueChangeRemoval)
            {
                [self removeSubtitlesTracksObservers:change[NSKeyValueChangeOldKey]];
            }
        }
        else if ([keyPath isEqualToString:@"container"] && change[NSKeyValueChangeNewKey] && NSAppKitVersionNumber >= NSAppKitVersionNumber10_10)
        {

            if ([change[NSKeyValueChangeNewKey] integerValue] & 0x030000)
            {
                self.bottomOptionsConstrain.active = YES;
            }
            else
            {
                self.bottomOptionsConstrain.active = NO;
            }
        }
        [self updateTracks:nil];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)addAudioTracksObservers:(NSArray<HBAudioTrack *> *)tracks
{
    for (HBAudioTrack *track in tracks)
    {
        [track addObserver:self forKeyPath:@"encoder" options:0 context:HBSummaryViewControllerContext];
        [track addObserver:self forKeyPath:@"mixdown" options:0 context:HBSummaryViewControllerContext];
    }
}

- (void)removeAudioTracksObservers:(NSArray<HBAudioTrack *> *)tracks
{
    for (HBAudioTrack *track in tracks)
    {
        [track removeObserver:self forKeyPath:@"encoder"];
        [track removeObserver:self forKeyPath:@"mixdown"];
    }
}

- (void)addSubtitlesTracksObservers:(NSArray<HBSubtitlesTrack *> *)tracks
{
    for (HBSubtitlesTrack *track in tracks)
    {
        [track addObserver:self forKeyPath:@"burnedIn" options:0 context:HBSummaryViewControllerContext];
    }
}

- (void)removeSubtitlesTracksObservers:(NSArray<HBSubtitlesTrack *> *)tracks
{
    for (HBSubtitlesTrack *track in tracks)
    {
        [track removeObserver:self forKeyPath:@"burnedIn"];
    }
}

- (void)addJobObservers
{
    if (_job)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updatePicture:) name:HBPictureChangedNotification object:_job.picture];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateFilters:) name:HBFiltersChangedNotification object:_job.filters];

        [_job addObserver:self forKeyPath:@"container" options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew context:HBSummaryViewControllerContext];
        [_job addObserver:self forKeyPath:@"video.encoder" options:0 context:HBSummaryViewControllerContext];
        [_job addObserver:self forKeyPath:@"video.frameRate" options:0 context:HBSummaryViewControllerContext];
        [_job addObserver:self forKeyPath:@"video.frameRateMode" options:0 context:HBSummaryViewControllerContext];
        [_job addObserver:self forKeyPath:@"container" options:0 context:HBSummaryViewControllerContext];
        [_job addObserver:self forKeyPath:@"chaptersEnabled" options:0 context:HBSummaryViewControllerContext];
        [_job addObserver:self forKeyPath:@"audio.tracks" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:HBSummaryViewControllerContext];
        [_job addObserver:self forKeyPath:@"subtitles.tracks" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:HBSummaryViewControllerContext];

        [self addAudioTracksObservers:_job.audio.tracks];
        [self addSubtitlesTracksObservers:_job.subtitles.tracks];
    }
}

- (void)removeJobObservers
{
    if (_job)
    {
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBPictureChangedNotification object:_job.picture];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBFiltersChangedNotification object:_job.filters];

        [_job removeObserver:self forKeyPath:@"container"];
        [_job removeObserver:self forKeyPath:@"video.encoder"];
        [_job removeObserver:self forKeyPath:@"video.frameRate"];
        [_job removeObserver:self forKeyPath:@"video.frameRateMode"];
        [_job removeObserver:self forKeyPath:@"container"];
        [_job removeObserver:self forKeyPath:@"chaptersEnabled"];
        [_job removeObserver:self forKeyPath:@"audio.tracks"];
        [_job removeObserver:self forKeyPath:@"subtitles.tracks"];

        [self removeAudioTracksObservers:_job.audio.tracks];
        [self removeSubtitlesTracksObservers:_job.subtitles.tracks];
    }
}

- (void)updateTracks:(NSNotification *)notification
{
    if (self.tracksReloadInQueue == NO)
    {
        [[NSRunLoop mainRunLoop] performSelector:@selector(updateTracksLabel) target:self argument:nil order:0 modes:@[NSDefaultRunLoopMode]];
        self.tracksReloadInQueue = YES;
    }
}

- (void)updateFilters:(NSNotification *)notification
{
    if (self.filtersReloadInQueue == NO)
    {
        [[NSRunLoop mainRunLoop] performSelector:@selector(updateFiltersLabel) target:self argument:nil order:0 modes:@[NSDefaultRunLoopMode]];
        self.filtersReloadInQueue = YES;
    }
}

- (void)updatePicture:(NSNotification *)notification
{
    // Enquee the reload call on the main runloop
    // to avoid reloading the same image multiple times.
    if (self.pictureReloadInQueue == NO)
    {
        [[NSRunLoop mainRunLoop] performSelector:@selector(updatePicture) target:self argument:nil order:0 modes:@[NSDefaultRunLoopMode]];
        self.pictureReloadInQueue = YES;
    }
}

- (void)resetLabels
{
    self.tracksLabel.stringValue = NSLocalizedString(@"None", nil);
    self.filtersLabel.stringValue = NSLocalizedString(@"None", nil);
    self.dimensionLabel.stringValue = NSLocalizedString(@"None", nil);
    self.tracksReloadInQueue = NO;
    self.filtersReloadInQueue = NO;
    self.pictureReloadInQueue = NO;
}

- (void)updateTracksLabel
{
    self.tracksLabel.stringValue = self.job.shortDescription;
    self.tracksReloadInQueue = NO;
}

- (void)updateFiltersLabel
{
    self.filtersLabel.stringValue = self.job.filtersShortDescription;
    self.filtersReloadInQueue = NO;
}

- (void)updatePicture
{
    if (self.visible && self.generator)
    {
        NSUInteger index = self.generator.imagesCount > 1 ? 1 : 0;
        CGImageRef fPreviewImage = [self.generator copyImageAtIndex:index shouldCache:NO];
        self.previewView.image = fPreviewImage;
        CFRelease(fPreviewImage);
        self.pictureReloadInQueue = NO;

        self.dimensionLabel.stringValue = self.job.picture.shortInfo;
    }
}

@end
