/*  HBQueueDetailsViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueDetailsViewController.h"

@interface HBQueueDetailsViewController ()

@property (weak) IBOutlet NSTextField *detailsLabel;
@property (weak) IBOutlet NSScrollView *scrollView;

@property (weak) id<HBQueueDetailsViewControllerDelegate> delegate;

@end

@implementation HBQueueDetailsViewController

- (NSString *)nibName
{
    return @"HBQueueDetailsViewController";
}

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
    self.item = nil;
}

- (void)setItem:(HBQueueItem *)item
{
    _item = item;
    if (item)
    {
        self.detailsLabel.attributedStringValue = item.attributedDescription;
        [self.scrollView flashScrollers];
    }
    else
    {
        self.detailsLabel.stringValue = NSLocalizedString(@"No job selected", @"");
    }
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
