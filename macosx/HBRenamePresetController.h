/*  HBRenamePresetController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class HBPreset;
@class HBPresetsManager;

@interface HBRenamePresetController : NSWindowController

- (instancetype)initWithPreset:(HBPreset *)preset presetManager:(HBPresetsManager *)manager;

@property (nonatomic, readonly) HBPreset *preset;

@end

NS_ASSUME_NONNULL_END
