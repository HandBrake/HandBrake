/*  HBQueueDetailsViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueInfoViewController.h"
#import "HBQueue.h"

@interface HBQueueInfoViewController ()

@property (nonatomic, weak) IBOutlet NSView *statisticsHeader;
@property (nonatomic, weak) IBOutlet NSTextField *statisticsLabel;
@property (nonatomic, weak) IBOutlet NSTextField *summaryLabel;
@property (nonatomic, weak) IBOutlet NSScrollView *scrollView;

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

- (void)viewDidLoad {
    [super viewDidLoad];
    [self updateLabels];
    [self setUpObservers];
}

- (void)setUpObservers
{
    // It would be easier to just KVO the item state property,
    // But we can't because the item is a NSProxy.
    NSNotificationCenter * __weak center = NSNotificationCenter.defaultCenter;

    [center addObserverForName:HBQueueDidStartItemNotification
                                              object:nil
                                               queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note)
                           {
                               HBQueueItem *startedItem = note.userInfo[HBQueueItemNotificationItemKey];

                               if (startedItem == self.item)
                               {
                                   [self updateLabels];
                                   [self updateReset];
                               }
                           }];

    [center addObserverForName:HBQueueDidCompleteItemNotification
                        object:nil
                         queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note)
     {
         HBQueueItem *completedItem = note.userInfo[HBQueueItemNotificationItemKey];

         if (completedItem == self.item)
         {
             [self updateLabels];
             [self updateReset];
         }
     }];

    [center addObserverForName:HBQueueDidChangeItemNotification
                        object:nil
                         queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note)
     {
         [self updateLabels];
         [self updateReset];
     }];

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

        [self.scrollView flashScrollers];
    }
    else
    {
        self.statisticsHeader.hidden = YES;
        self.statisticsLabel.hidden = YES;
        self.summaryLabel.hidden = YES;
    }
}

- (void)setItem:(HBQueueItem *)item
{
    _item = item;
    [self updateLabels];
    [self updateReset];
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
