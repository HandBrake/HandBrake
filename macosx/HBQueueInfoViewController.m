/*  HBQueueDetailsViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueInfoViewController.h"

@interface HBQueueInfoViewController ()

@property (weak) IBOutlet NSTextField *summaryLabel;
@property (weak) IBOutlet NSTextField *statisticsLabel;
@property (weak) IBOutlet NSScrollView *scrollView;

@property (weak) id<HBQueueDetailsViewControllerDelegate> delegate;

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

- (void)viewDidLoad {
    [super viewDidLoad];
    [self updateLabels];
}

- (void)updateLabels
{
    if (self.item)
    {
        self.statisticsLabel.hidden = self.item.endedDate == nil;
        self.summaryLabel.hidden = NO;

        self.statisticsLabel.attributedStringValue = self.item.attributedStatistics;
        self.summaryLabel.attributedStringValue = self.item.attributedDescription;

        [self.scrollView flashScrollers];
    }
    else
    {
        self.statisticsLabel.hidden = YES;
        self.summaryLabel.hidden = YES;
    }
}

- (void)setItem:(HBQueueItem *)item
{
    _item = item;
    [self updateLabels];
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
