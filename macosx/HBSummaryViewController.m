/*  HBSummaryViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSummaryViewController.h"
#import "HBPreviewView.h"
#import "HBPreviewGenerator.h"

@import HandBrakeKit;

@interface HBSummaryViewController ()

@property (strong) IBOutlet HBPreviewView *previewView;
@property (strong) IBOutlet NSTextField *tracksLabel;
@property (strong) IBOutlet NSTextField *filtersLabel;

@end

@implementation HBSummaryViewController

- (void)loadView {
    [super loadView];
    self.previewView.showShadow = NO;
    [self resetLabels];
}

- (void)setGenerator:(HBPreviewGenerator *)generator
{
    _generator = generator;

    if (generator)
    {
        NSUInteger index = generator.imagesCount > 1 ? 1 : 0;
        CGImageRef fPreviewImage = [generator copyImageAtIndex:index shouldCache:NO];
        self.previewView.image = fPreviewImage;
        CFRelease(fPreviewImage);
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
        _job = job;
        [self addJobObservers];
        [self updateLabels];
    }
    else
    {
        [self removeJobObservers];
        [self resetLabels];
        _job = job;
    }
}

- (void)removeJobObservers
{
    if (self.job)
    {
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBContainerChangedNotification object:_job];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBPictureChangedNotification object:_job.picture];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBFiltersChangedNotification object:_job.filters];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBVideoChangedNotification object:_job.video];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBAudioChangedNotification object:_job.audio];
        [[NSNotificationCenter defaultCenter] removeObserver:self name:HBChaptersChangedNotification object:_job];
    }
}

- (void)updateTracks:(NSNotification *)notification
{
    self.tracksLabel.stringValue = self.job.shortDescription;
}

- (void)updateFilters:(NSNotification *)notification
{
    self.filtersLabel.stringValue = self.job.filtersShortDescription;
}

- (void)resetLabels
{
    self.tracksLabel.stringValue = NSLocalizedString(@"None", nil);
    self.filtersLabel.stringValue = NSLocalizedString(@"None", nil);
}

- (void)updateLabels
{
    self.tracksLabel.stringValue = self.job.shortDescription;
    self.filtersLabel.stringValue = self.job.filtersShortDescription;
}

- (void)addJobObservers
{
    if (self.job)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateTracks:) name:HBContainerChangedNotification object:_job];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateTracks:) name:HBPictureChangedNotification object:_job.picture];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateFilters:) name:HBFiltersChangedNotification object:_job.filters];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateTracks:) name:HBVideoChangedNotification object:_job.video];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateTracks:) name:HBAudioChangedNotification object:_job.audio];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateTracks:) name:HBChaptersChangedNotification object:_job];
    }
}

@end
