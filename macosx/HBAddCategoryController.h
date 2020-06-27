/*  HBAddCategoryController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

@interface HBAddCategoryController : NSWindowController

- (instancetype)initWithPresetManager:(HBPresetsManager *)manager;

@property (nonatomic, readonly, nullable) HBPreset *category;

@end

NS_ASSUME_NONNULL_END
