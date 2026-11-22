/*  HBJob+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

#define HB_AUTO_INCREMENT_PAD_MIN 2
#define HB_AUTO_INCREMENT_PAD_MAX 5

@interface HBJob (HBAdditions)

/// Generates a file name automatically based on the inputs,
/// it can be configured with NSUserDefaults
@property (nonatomic, readonly) NSString *automaticName;
@property (nonatomic, readonly) NSString *automaticExt;
@property (nonatomic, readonly) NSString *defaultName;

/// Same as automaticName, but the {Auto-Increment} token renders one past
/// the highest number already used by a matching file in the destination
/// folder or by any of the reserved destinations (e.g. queued jobs).
/// If currentName already holds an unused number at or below that value,
/// the number is kept, so a job being edited does not jump to a new number.
- (NSString *)automaticNameReservingURLs:(nullable NSArray<NSURL *> *)reservedURLs
                             currentName:(nullable NSString *)currentName;

/// Same as defaultName, with {Auto-Increment} resolved as described above.
- (NSString *)defaultNameReservingURLs:(nullable NSArray<NSURL *> *)reservedURLs;

/// Whether automatic naming is enabled and the format uses {Auto-Increment}
+ (BOOL)autoIncrementEnabled;

- (void)setDestinationFolderURL:(NSURL *)destinationFolderURL sameAsSource:(BOOL)useSourceFolderDestination;

@end

NS_ASSUME_NONNULL_END
