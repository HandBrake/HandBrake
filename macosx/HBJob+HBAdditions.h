/*  HBJob+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

@interface HBJob (HBAdditions)

/// Generates a file name automatically based on the inputs,
/// it can be configured with NSUserDefaults
@property (nonatomic, readonly) NSString *automaticName;
@property (nonatomic, readonly) NSString *automaticExt;
@property (nonatomic, readonly) NSString *defaultName;

@end

NS_ASSUME_NONNULL_END
