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

/// Same as defaultName, but the {Auto-Increment} token renders
/// offset positions past the next stored counter value, so a batch
/// of titles can be numbered consecutively
- (NSString *)defaultNameWithAutoIncrementOffset:(NSUInteger)offset;

/// Whether automatic naming is enabled and the format uses {Auto-Increment}
+ (BOOL)autoIncrementEnabled;

/// Advances the stored {Auto-Increment} counter after count jobs
/// have been committed to the queue, rolling over to 1 past the
/// highest value that fits in the configured padding
+ (void)advanceAutoIncrementBy:(NSUInteger)count;

- (void)setDestinationFolderURL:(NSURL *)destinationFolderURL sameAsSource:(BOOL)useSourceFolderDestination;

@end

NS_ASSUME_NONNULL_END
