/* HBQueueDockTileController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueDockTileController.h"

#import "HBDockTile.h"
#import "HBQueue.h"

#define dockTileUpdateFrequency 0.1f

@interface HBQueueDockTileController ()

@property (nonatomic, readonly) HBQueue *queue;
@property (nonatomic, readonly) HBDockTile *dockTile;
@property (nonatomic) double progress;

@property (nonatomic) id observerToken;

@end

@implementation HBQueueDockTileController

- (instancetype)initWithQueue:(HBQueue *)queue dockTile:(id)dockTile image:(NSImage *)image
{
    self = [super init];
    if (self)
    {
        _queue = queue;
        _dockTile = [[HBDockTile alloc] initWithDockTile:dockTile image:image];

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidStartItemNotification
                                                        object:_queue queue:NSOperationQueue.mainQueue
                                                    usingBlock:^(NSNotification * _Nonnull note) { [self setUpObservers]; }];

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidCompleteItemNotification
                                                        object:_queue queue:NSOperationQueue.mainQueue
                                                    usingBlock:^(NSNotification * _Nonnull note) { [self setUpObservers]; }];

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidChangeStateNotification
                                                        object:_queue queue:NSOperationQueue.mainQueue
                                                    usingBlock:^(NSNotification * _Nonnull note) { [self setUpObservers]; }];

        [NSNotificationCenter.defaultCenter addObserverForName:HBQueueDidCompleteNotification
                                                        object:_queue queue:NSOperationQueue.mainQueue
                                                    usingBlock:^(NSNotification * _Nonnull note) { self.dockTile.stringValue = @""; }];
    }
    return self;
}

- (void)setUpObservers
{
    [self removeObservers];

    if (self->_queue.workingItemsCount > 1)
    {
        [self setUpForMultipleWorkers];
    }
    else if (self->_queue.workingItemsCount == 1)
    {
        [self setUpForSingleWorker];
    }
}

- (void)setUpForMultipleWorkers
{
    self.dockTile.stringValue = [NSString stringWithFormat:@"%lu - %lu", self.queue.workingItemsCount, self.queue.pendingItemsCount];
    self.progress = 0;
}

- (void)setUpForSingleWorker
{
    self.progress = 0;
    HBQueueJobItem *firstWorkingItem = nil;
    for (HBQueueJobItem *item in self.queue.items)
    {
        if (item.state == HBQueueItemStateWorking)
        {
            firstWorkingItem = item;
            break;
        }
    }

    if (firstWorkingItem)
    {
        HBQueueWorker *worker = [self.queue workerForItem:firstWorkingItem];

        if (worker)
        {

            self.observerToken = [NSNotificationCenter.defaultCenter addObserverForName:HBQueueWorkerProgressNotification
                                                                                 object:worker queue:NSOperationQueue.mainQueue
                                                                             usingBlock:^(NSNotification * _Nonnull note) {
                double progress = [note.userInfo[HBQueueWorkerProgressNotificationPercentKey] doubleValue];

                if (self.progress < 100.0 * progress)
                {
                    double hours = [note.userInfo[HBQueueWorkerProgressNotificationHoursKey] doubleValue];
                    double minutes = [note.userInfo[HBQueueWorkerProgressNotificationMinutesKey] doubleValue];
                    double seconds = [note.userInfo[HBQueueWorkerProgressNotificationSecondsKey] doubleValue];

                    [self.dockTile setProgress:progress hours:hours minutes:minutes seconds:seconds];
                    self.progress += dockTileUpdateFrequency;
                }
            }];
        }
    }
}

- (void)removeObservers
{
    if (self.observerToken)
    {
        [NSNotificationCenter.defaultCenter removeObserver:self.observerToken];
        self.observerToken = nil;
    }
}

@end
