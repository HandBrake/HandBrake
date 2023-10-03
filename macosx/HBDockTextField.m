/*  HBDockTextField.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBDockTextField.h"

#define DOCK_TEXTFIELD_ALPHA 0.8
#define DOCK_TEXTFIELD_FONTSIZE 28.0

@interface HBDockTextField ()

@property (nonatomic, readonly) NSDictionary *textAttributes;
@property (nonatomic, readonly) NSDictionary *smallTextAttributes;

@property (nonatomic, readwrite) NSGradient *gradient;

@end

@implementation HBDockTextField

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [[self cell] setBezelStyle:NSBezelStyleRounded];
        _textAttributes = [self textAttributesWithFontSize:DOCK_TEXTFIELD_FONTSIZE];
        _smallTextAttributes = [self textAttributesWithFontSize:DOCK_TEXTFIELD_FONTSIZE - 2];
        [self changeGradientColors:NSColor.grayColor endColor:NSColor.blackColor];
    }

    return self;
}

- (NSDictionary *)textAttributesWithFontSize:(CGFloat)fontSize
{
    NSShadow *shadow = [[NSShadow alloc] init];
    shadow.shadowColor = NSColor.blackColor;
    shadow.shadowOffset = NSMakeSize(2, -2);
    shadow.shadowBlurRadius = 6;

    NSFont *font = [NSFont monospacedDigitSystemFontOfSize:fontSize weight:NSFontWeightBold];

    return @{ NSForegroundColorAttributeName: NSColor.whiteColor,
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
    {
        return;
    }

    const NSSize size = self.bounds.size;
    const NSRect blackOutlineFrame = NSMakeRect(0.0, 0.0, size.width, size.height - 1.0);
    const double radius = size.height / 2;
    NSBezierPath *bezierPath = [NSBezierPath bezierPathWithRoundedRect:blackOutlineFrame
                                                               xRadius:radius yRadius:radius];

    [self.gradient drawInBezierPath:bezierPath angle:90];

    NSDictionary *attributes = self.textAttributes;
    NSString *budgetString = self.stringValue;
	NSSize stringSize = [budgetString sizeWithAttributes:attributes];

    if (size.width - 4 < stringSize.width)
    {
        attributes = self.smallTextAttributes;
        stringSize = [budgetString sizeWithAttributes:attributes];
    }

	NSPoint centerPoint;
	centerPoint.x = (size.width / 2) - (stringSize.width / 2);
	centerPoint.y = size.height / 2 - (stringSize.height / 2) - 2;

	[budgetString drawAtPoint:centerPoint withAttributes:attributes];
}

@end
