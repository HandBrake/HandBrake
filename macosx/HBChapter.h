/*  HBChapter.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBChapter : NSObject <NSSecureCoding, NSCopying>

- (instancetype)initWithTitle:(NSString *)title index:(NSUInteger)idx duration:(uint64_t)duration NS_DESIGNATED_INITIALIZER;

@property (nonatomic, readwrite) NSString *title;
@property (nonatomic, readonly) NSString *duration;
@property (nonatomic, readwrite) NSUInteger index;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
