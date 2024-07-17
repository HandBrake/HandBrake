/* HBTitleSelectionController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBTitleSelectionRange.h"

NS_ASSUME_NONNULL_BEGIN

@class HBTitle;

@protocol HBTitleSelectionDelegate <NSObject>

- (void)didSelectTitles:(NSArray<HBTitle *> *)titles range:(nullable HBTitleSelectionRange *)range;

@end

@interface HBTitleSelectionController : NSWindowController

- (instancetype)initWithTitles:(NSArray<HBTitle *> *)titles presetName:(NSString *)presetName delegate:(id<HBTitleSelectionDelegate>)delegate;

@end

NS_ASSUME_NONNULL_END
