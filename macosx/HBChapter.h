/*  HBChapter.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBChapter : NSObject <NSSecureCoding, NSCopying>

- (instancetype)initWithTitle:(NSString *)title index:(NSUInteger)idx timestamp:(uint64_t)timestamp duration:(uint64_t)duration NS_DESIGNATED_INITIALIZER;

@property (nonatomic, readwrite, copy) NSString *title;
@property (nonatomic, readonly) uint64_t duration;
@property (nonatomic, readonly) uint64_t timestamp;
@property (nonatomic, readonly) NSUInteger index;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
