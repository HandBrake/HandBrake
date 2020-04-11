/*  HBQueueItemWorkingView.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueItemView.h"

@class HBQueueWorker;

NS_ASSUME_NONNULL_BEGIN

@interface HBQueueItemWorkingView : HBQueueItemView

@property (nonatomic) HBQueueWorker *worker;

@end

NS_ASSUME_NONNULL_END
