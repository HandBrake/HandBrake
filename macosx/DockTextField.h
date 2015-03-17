/*  DockTextField.h $
 
 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@interface DockTextField : NSTextField

@property (nonatomic,strong) NSString *textToDisplay;
@property (nonatomic,strong) NSColor *startColor;
@property (nonatomic,strong) NSColor *endColor;

- (void)changeGradientColors:(NSColor*)startColor endColor:(NSColor*)endColor;

@end
