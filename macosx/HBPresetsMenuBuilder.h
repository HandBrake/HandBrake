/*  HBPresetsMenuBuilder.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@import HandBrakeKit;

@interface HBPresetsMenuBuilder : NSObject

@property (nonatomic, readonly) NSMenu *menu;

- (instancetype)initWithMenu:(NSMenu *)menu action:(SEL)action size:(CGFloat)size presetsManager:(HBPresetsManager *)manager;

/**
 *  Adds the presets list to the menu.
 */
- (void)build;

@end
