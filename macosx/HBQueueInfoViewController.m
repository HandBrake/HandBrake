/*  HBQueueDetailsViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueInfoViewController.h"

static void *HBQueueInfoViewControllerContext = &HBQueueInfoViewControllerContext;

@interface HBQueueInfoViewController ()

@property (nonatomic, weak) IBOutlet NSView *statisticsHeader;
@property (nonatomic, weak) IBOutlet NSTextField *statisticsLabel;
@property (nonatomic, weak) IBOutlet NSTextField *summaryLabel;
@property (nonatomic, weak) IBOutlet NSScrollView *scrollView;
@property (nonatomic, weak) IBOutlet NSBox *divider;

@property (nonatomic, weak) id<HBQueueDetailsViewControllerDelegate> delegate;

@property (nonatomic) BOOL canReset;

@end

@implementation HBQueueInfoViewController

- (instancetype)initWithDelegate:(id<HBQueueDetailsViewControllerDelegate>)delegate
{
    self = [super init];
    if (self)
    {
        _delegate = delegate;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self updateLabels];
    [self addObserver:self forKeyPath:@"item.state" options:0 context:HBQueueInfoViewControllerContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBQueueInfoViewControllerContext)
    {
        [self updateUI];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}


- (void)updateUI
{
    [self updateLabels];
    [self updateReset];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self updateDivider];
    });
}

- (void)updateReset
{
    self.canReset = self.item && (self.item.state != HBQueueItemStateWorking && self.item.state != HBQueueItemStateReady);
}

- (void)updateLabels
{
    if (self.item)
    {
        self.statisticsLabel.hidden = self.item.startedDate == nil;
        self.statisticsHeader.hidden = self.item.startedDate == nil;
        self.summaryLabel.hidden = NO;

        self.statisticsLabel.attributedStringValue = self.item.attributedStatistics;
        self.summaryLabel.attributedStringValue = self.item.attributedDescription;

        dispatch_async(dispatch_get_main_queue(), ^{
            [self.scrollView flashScrollers];
        });
    }
    else
    {
        self.statisticsHeader.hidden = YES;
        self.statisticsLabel.hidden = YES;
        self.summaryLabel.hidden = YES;
    }
}

- (void)updateDivider
{
    self.divider.hidden = self.scrollView.frame.size.height > self.scrollView.contentView.documentView.frame.size.height;
}

- (void)viewDidLayout
{
    [super viewDidLayout];
    [self updateDivider];
}

- (void)setItem:(HBQueueItem *)item
{
    _item = item;
}

- (IBAction)editItem:(id)sender
{
    [self.delegate detailsViewEditItem:self.item];
}

- (IBAction)resetItem:(id)sender
{
    [self.delegate detailsViewResetItem:self.item];
}

@end
