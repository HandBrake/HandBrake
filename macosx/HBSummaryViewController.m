/*  HBSummaryViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSummaryViewController.h"

#import "HBPreviewViewController.h"
#import "HBPreviewGenerator.h"

@import HandBrakeKit;

static void *HBSummaryViewControllerContainerContext = &HBSummaryViewControllerContainerContext;
static void *HBSummaryViewControllerVideoContext = &HBSummaryViewControllerVideoContext;
static void *HBSummaryViewControllerPictureContext = &HBSummaryViewControllerPictureContext;
static void *HBSummaryViewControllerFiltersContext = &HBSummaryViewControllerFiltersContext;
static void *HBSummaryViewControllerAudioContext = &HBSummaryViewControllerAudioContext;
static void *HBSummaryViewControllerSubsContext = &HBSummaryViewControllerSubsContext;

@interface HBSummaryViewController ()

@property (nonatomic, strong) IBOutlet NSLayoutConstraint *bottomOptionsConstrain;

@property (nonatomic, strong) IBOutlet NSTextField *tracksLabel;
@property (nonatomic, strong) IBOutlet NSTextField *filtersLabel;
@property (nonatomic, strong) IBOutlet NSTextField *dimensionLabel;

@property (nonatomic, strong) IBOutlet NSView *previewView;

@property (nonatomic, strong) HBPreviewViewController *previewViewController;

@property (nonatomic) BOOL tracksReloadInQueue;
@property (nonatomic) BOOL filtersReloadInQueue;
@property (nonatomic) BOOL pictureReloadInQueue;

@property (nonatomic, readwrite) NSColor *labelColor;

@end

@implementation HBSummaryViewController

- (instancetype)init
{
    self = [super initWithNibName:@"HBSummaryViewController" bundle:nil];
    if (self)
    {
        _labelColor = [NSColor disabledControlTextColor];

        _previewViewController = [[HBPreviewViewController alloc] init];
    }
    return self;
}

- (void)loadView
{
    [super loadView];
    self.previewViewController.view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    self.previewViewController.view.frame = NSMakeRect(0, 0, self.previewView.frame.size.width, self.previewView.frame.size.height);
    [self.previewView addSubview:self.previewViewController.view];
    [self resetLabels];
}

- (void)setGenerator:(HBPreviewGenerator *)generator
{
    self.previewViewController.generator = generator;
}

- (void)setJob:(HBJob *)job
{
    if (job)
    {
        self.labelColor = [NSColor controlTextColor];
        [self removeJobObservers];
        _job = job;
        [self addJobObservers];
        [self updateTracksLabel];
        [self updateFiltersLabel];
        [self updatePictureLabel];
    }
    else
    {
        self.labelColor = [NSColor disabledControlTextColor];
        [self removeJobObservers];
        [self resetLabels];
        _job = job;
    }
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBSummaryViewControllerAudioContext)
    {
        if ([change[NSKeyValueChangeKindKey] integerValue] == NSKeyValueChangeInsertion)
        {
            [self addAudioTracksObservers:change[NSKeyValueChangeNewKey]];
        }
        else if ([change[NSKeyValueChangeKindKey] integerValue]== NSKeyValueChangeRemoval)
        {
            [self removeAudioTracksObservers:change[NSKeyValueChangeOldKey]];
        }
        [self updateTracks:nil];
    }
    else if (context == HBSummaryViewControllerSubsContext)
    {
        if ([change[NSKeyValueChangeKindKey] integerValue] == NSKeyValueChangeInsertion)
        {
            [self addSubtitlesTracksObservers:change[NSKeyValueChangeNewKey]];
        }
        else if ([change[NSKeyValueChangeKindKey] integerValue]== NSKeyValueChangeRemoval)
        {
            [self removeSubtitlesTracksObservers:change[NSKeyValueChangeOldKey]];
        }
        [self updateTracks:nil];
    }
    else if (context == HBSummaryViewControllerContainerContext)
    {
        if (NSAppKitVersionNumber >= NSAppKitVersionNumber10_10 && change[NSKeyValueChangeNewKey])
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
    else if (context == HBSummaryViewControllerVideoContext)
    {
        [self updateTracks:nil];
    }
    else if (context == HBSummaryViewControllerFiltersContext)
    {
        [self updatePicture:nil];
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
        [track addObserver:self forKeyPath:@"encoder" options:0 context:HBSummaryViewControllerAudioContext];
        [track addObserver:self forKeyPath:@"mixdown" options:0 context:HBSummaryViewControllerAudioContext];
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
        [track addObserver:self forKeyPath:@"burnedIn" options:0 context:HBSummaryViewControllerSubsContext];
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

        [_job addObserver:self forKeyPath:@"container" options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew context:HBSummaryViewControllerContainerContext];
        [_job addObserver:self forKeyPath:@"chaptersEnabled" options:0 context:HBSummaryViewControllerVideoContext];
        [_job addObserver:self forKeyPath:@"video.encoder" options:0 context:HBSummaryViewControllerVideoContext];
        [_job addObserver:self forKeyPath:@"video.frameRate" options:0 context:HBSummaryViewControllerVideoContext];
        [_job addObserver:self forKeyPath:@"video.frameRateMode" options:0 context:HBSummaryViewControllerVideoContext];
        [_job addObserver:self forKeyPath:@"filters.deinterlace" options:0 context:HBSummaryViewControllerFiltersContext];
        [_job addObserver:self forKeyPath:@"filters.rotate" options:0 context:HBSummaryViewControllerFiltersContext];
        [_job addObserver:self forKeyPath:@"filters.flip" options:0 context:HBSummaryViewControllerFiltersContext];
        [_job addObserver:self forKeyPath:@"audio.tracks" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:HBSummaryViewControllerAudioContext];
        [_job addObserver:self forKeyPath:@"subtitles.tracks" options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld context:HBSummaryViewControllerSubsContext];

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
        [_job removeObserver:self forKeyPath:@"chaptersEnabled"];
        [_job removeObserver:self forKeyPath:@"video.encoder"];
        [_job removeObserver:self forKeyPath:@"video.frameRate"];
        [_job removeObserver:self forKeyPath:@"video.frameRateMode"];
        [_job removeObserver:self forKeyPath:@"filters.deinterlace"];
        [_job removeObserver:self forKeyPath:@"filters.rotate"];
        [_job removeObserver:self forKeyPath:@"filters.flip"];
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
        [[NSRunLoop mainRunLoop] performSelector:@selector(updatePictureLabel) target:self argument:nil order:0 modes:@[NSDefaultRunLoopMode]];
        self.pictureReloadInQueue = YES;
    }
}

- (void)resetLabels
{
    self.tracksLabel.stringValue = @"";
    self.filtersLabel.stringValue = @"";
    self.dimensionLabel.stringValue = @"";
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

- (void)updatePictureLabel
{
    self.pictureReloadInQueue = NO;
    self.dimensionLabel.stringValue = self.job.picture.shortInfo;
    [self.previewViewController update];
}

@end
