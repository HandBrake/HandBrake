/*  HBAddPresetController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@import HandBrakeKit;

@interface HBAddPresetController : NSWindowController

- (instancetype)initWithPreset:(HBPreset *)preset presetManager:(HBPresetsManager *)manager customWidth:(int)customWidth customHeight:(int)customHeight resolutionLimitMode:(HBPictureResolutionLimitMode)resolutionLimitMode;

@property (nonatomic, readonly) HBPreset *preset;

@end

NS_ASSUME_NONNULL_END
