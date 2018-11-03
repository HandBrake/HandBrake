/*  HBAutoNamer.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class HBJob;

@interface HBAutoNamer : NSObject

- (instancetype)initWithJob:(HBJob *)job;

- (void)updateFileExtension;
- (void)updateFileName;

@end

NS_ASSUME_NONNULL_END
