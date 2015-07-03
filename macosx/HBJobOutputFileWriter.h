/*  HBJobOutputFileWriter.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBOutputFileWriter.h"

NS_ASSUME_NONNULL_BEGIN

@class HBJob;

/**
 * Redirects the output to a new file based on the job destination
 * and the current logging preference.
 */
@interface HBJobOutputFileWriter : HBOutputFileWriter

- (nullable instancetype)initWithJob:(HBJob *)job NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
