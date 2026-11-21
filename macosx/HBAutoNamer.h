/*  HBAutoNamer.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class HBJob;
@class HBQueue;

@interface HBAutoNamer : NSObject

/// The queue's destinations are taken into account when numbering
/// the {Auto-Increment} token, and the name refreshes when it changes.
- (instancetype)initWithJob:(HBJob *)job queue:(nullable HBQueue *)queue;

- (void)updateFileExtension;
- (void)updateFileName;

@end

NS_ASSUME_NONNULL_END
