/*  HBQueueArchive.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBQueueItem.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HBQueueArchiveFileExtension;

/// Reads and writes the queue files the user exports.
@interface HBQueueArchive : NSObject

+ (BOOL)writeItems:(NSArray<id<HBQueueItem>> *)items toURL:(NSURL *)url error:(NSError **)outError;

/// Returns the items ready to be encoded again, or nil if the file
/// is not a queue or was written by an incompatible version.
+ (nullable NSArray<id<HBQueueItem>> *)itemsWithContentsOfURL:(NSURL *)url error:(NSError **)outError;

@end

NS_ASSUME_NONNULL_END
