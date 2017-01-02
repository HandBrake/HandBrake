/*  DockTextField.h $
 
 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@interface DockTextField : NSTextField

@property (nonatomic, copy) NSString *textToDisplay;
@property (nonatomic, copy) NSColor *startColor;
@property (nonatomic, copy) NSColor *endColor;

- (void)changeGradientColors:(NSColor *)startColor endColor:(NSColor *)endColor;

@end
