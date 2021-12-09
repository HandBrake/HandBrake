/* HBPasteboardItem.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

extern NSPasteboardType const tableViewIndex;

@interface HBPasteboardItem : NSObject<NSPasteboardWriting>

- (instancetype)initWithIndex:(NSInteger)index;

@property (nonatomic, readonly) NSInteger index;

@end

NS_ASSUME_NONNULL_END
