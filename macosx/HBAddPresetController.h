/*  HBAddPresetController.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class HBPreset;

@interface HBAddPresetController : NSWindowController

- (instancetype)initWithPreset:(HBPreset *)preset customWidth:(int)customWidth customHeight:(int)customHeight defaultToCustom:(BOOL)defaultToCustom;

@property (nonatomic, readonly) HBPreset *preset;

@end

NS_ASSUME_NONNULL_END
