/* HBTitleSelectionRangeController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBTitleSelectionRange.h"

NS_ASSUME_NONNULL_BEGIN

@interface HBTitleSelectionRangeController : NSViewController

@property (nonatomic) HBTitleSelectionRange *range;
@property (nonatomic) BOOL enabled;

- (instancetype)initWithTitles:(NSArray<HBTitle *> *)titles;

@end

NS_ASSUME_NONNULL_END
