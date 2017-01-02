/*  DockTextField.m $
 
 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "DockTextField.h"

#define DOCK_TEXTFIELD_ALPHA 0.8
#define DOCK_TEXTFIELD_FONTSIZE 28.0

@interface DockTextField ()

@property (nonatomic, readonly) NSDictionary *textAttributes;
@property (nonatomic, readonly) NSDictionary *smallTextAttributes;

@property (nonatomic, readwrite) NSGradient *gradient;

@end

@implementation DockTextField

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [[self cell] setBezelStyle:NSRoundedBezelStyle];
        _textToDisplay = @"";
        _textAttributes = [self textAttributesWithFontSize:DOCK_TEXTFIELD_FONTSIZE];
        _smallTextAttributes = [self textAttributesWithFontSize:DOCK_TEXTFIELD_FONTSIZE - 2];
        [self changeGradientColors:[NSColor grayColor] endColor:[NSColor blackColor]];
    }
    
    return self;
}

- (NSDictionary *)textAttributesWithFontSize:(CGFloat)fontSize
{
    NSShadow *shadow = [[NSShadow alloc] init];
    shadow.shadowColor = [NSColor blackColor];
    shadow.shadowOffset = NSMakeSize(2, -2);
    shadow.shadowBlurRadius = 6;

    NSFont *font;
    if ([[NSFont class] respondsToSelector:@selector(monospacedDigitSystemFontOfSize:weight:)]) {
        // On macOS 10.11+ the monospaced digit system is available.
        font = [NSFont monospacedDigitSystemFontOfSize:fontSize weight:NSFontWeightBold];
    } else {
        // macOS 10.10- use the default system font.
        font = [NSFont boldSystemFontOfSize:fontSize];
    }

    return @{ NSForegroundColorAttributeName: [NSColor whiteColor],
              NSFontAttributeName: font,
              NSShadowAttributeName: shadow};
}

- (void)changeGradientColors:(NSColor *)startColor endColor:(NSColor *)endColor
{
    self.startColor = [startColor colorWithAlphaComponent:DOCK_TEXTFIELD_ALPHA];
    self.endColor = [endColor colorWithAlphaComponent:DOCK_TEXTFIELD_ALPHA];
    self.gradient = [[NSGradient alloc] initWithStartingColor:self.startColor endingColor:self.endColor];
}

- (void)drawRect:(NSRect)dirtyRect
{
    if (self.isHidden)
        return;

    NSSize size = self.bounds.size;
    NSRect blackOutlineFrame = NSMakeRect(0.0, 0.0, size.width, size.height - 1.0);
    double radius = self.bounds.size.height / 2;

    [self.gradient drawInBezierPath:[NSBezierPath bezierPathWithRoundedRect:blackOutlineFrame xRadius:radius yRadius:radius] angle:90];

    NSDictionary *attributes = self.textAttributes;
	NSString *budgetString = _textToDisplay;
	NSSize stringSize = [budgetString sizeWithAttributes:attributes];

    if (size.width - 4 < stringSize.width)
    {
        attributes = self.smallTextAttributes;
        stringSize = [budgetString sizeWithAttributes:attributes];
    }

	NSPoint centerPoint;
	centerPoint.x = (dirtyRect.size.width / 2) - (stringSize.width / 2);
	centerPoint.y = dirtyRect.size.height / 2 - (stringSize.height / 2) - 2;

	[budgetString drawAtPoint:centerPoint withAttributes:attributes];
}

@end
