/*  HBQueueItemWorkingView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueItemWorkingView.h"

#import "HBQueueJobItem.h"
#import "HBQueueWorker.h"

#import "HBAttributedStringAdditions.h"

@interface HBQueueItemWorkingView ()

@property (nonatomic, weak) IBOutlet NSProgressIndicator *progressBar;
@property (nonatomic, weak) IBOutlet NSTextField *progressField;

@property (nonatomic) id progressToken;
@property (nonatomic) id completedToken;

@end

@implementation HBQueueItemWorkingView

- (void)setUpObserversWithWorker:(HBQueueWorker *)worker
{
    NSNotificationCenter * __weak center = NSNotificationCenter.defaultCenter;

    self.progressToken = [center addObserverForName:HBQueueWorkerProgressNotification
                                             object:worker
                                              queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note)
                          {
                              NSString *progressInfo = note.userInfo[HBQueueWorkerProgressNotificationInfoKey];
                              double progress = [note.userInfo[HBQueueWorkerProgressNotificationPercentKey] doubleValue];

                              self.progressField.stringValue = progressInfo;
                              self.progressBar.doubleValue = progress;
                          }];

    self.completedToken = [center addObserverForName:HBQueueWorkerDidCompleteItemNotification
                                              object:worker
                                               queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note)
                           {
                                [self removeObservers];
                           }];

    self.progressField.font = [NSFont monospacedDigitSystemFontOfSize:NSFont.smallSystemFontSize weight:NSFontWeightRegular];
}

- (void)removeObservers
{
    if (self.progressToken)
    {
        [NSNotificationCenter.defaultCenter removeObserver:self.progressToken];
        self.progressToken = nil;
    }
    if (self.completedToken)
    {
        [NSNotificationCenter.defaultCenter removeObserver:self.completedToken];
        self.completedToken = nil;
    }
}

- (void)dealloc
{
    [self removeObservers];
}

- (void)setWorker:(HBQueueWorker *)worker
{
    [self setUpObserversWithWorker:worker];
}

- (void)setItem:(HBQueueJobItem *)item
{
    [self removeObservers];
    self.progressField.stringValue = @"";
    [super setItem:item];
}

@end
