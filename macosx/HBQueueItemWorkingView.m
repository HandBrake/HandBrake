/*  HBQueueItemWorkingView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueItemWorkingView.h"

#import "HBQueueItem.h"
#import "HBQueue.h"

@interface HBQueueItemWorkingView ()

@property (nonatomic, weak) IBOutlet NSProgressIndicator *progressBar;
@property (nonatomic, weak) IBOutlet NSTextField *progressField;

@property (nonatomic) id progressToken;
@property (nonatomic) id completedToken;

@end

@implementation HBQueueItemWorkingView

- (void)setItem:(HBQueueItem *)item
{
    [super setItem:item];

    if (item.state == HBQueueItemStateWorking)
    {
        NSNotificationCenter * __weak center = NSNotificationCenter.defaultCenter;

        self.progressToken = [center addObserverForName:HBQueueProgressNotification
                                                 object:nil
                                                  queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note)
        {
            NSString *progressInfo = note.userInfo[HBQueueProgressNotificationInfoKey];
            double progress = [note.userInfo[HBQueueProgressNotificationPercentKey] doubleValue];

            self.progressField.stringValue = progressInfo;
            self.progressBar.doubleValue = progress;
        }];

        self.completedToken = [center addObserverForName:HBQueueDidCompleteItemNotification
                                                  object:nil
                                                   queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note)
        {
            HBQueueItem *completedItem = note.userInfo[HBQueueItemNotificationItemKey];
            if (completedItem == self.item) {
                [center removeObserver:self.progressToken];
                [center removeObserver:self.completedToken];
            }
        }];
    }
}

@end
