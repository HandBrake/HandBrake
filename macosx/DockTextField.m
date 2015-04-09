/*  DockTextField.m $
 
 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "DockTextField.h"

#define DOCK_TEXTFIELD_ALPHA 0.8
#define DOCK_TEXTFIELD_FONTSIZE 28.0

@implementation DockTextField

@synthesize textToDisplay = _textToDisplay;
@synthesize startColor = _startColor;
@synthesize endColor = _endColor;

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [[self cell] setBezelStyle:NSRoundedBezelStyle];
        _textToDisplay = @"";
        [self changeGradientColors:[NSColor grayColor] endColor:[NSColor blackColor]];
    }
    
    return self;
}

- (void)changeGradientColors:(NSColor*)startColor endColor:(NSColor*)endColor
{
    self.startColor = [startColor colorWithAlphaComponent:DOCK_TEXTFIELD_ALPHA];
    self.endColor = [endColor colorWithAlphaComponent:DOCK_TEXTFIELD_ALPHA];
}

- (void)drawRect:(NSRect)dirtyRect
{
    if (self.isHidden)
        return;
    
    NSRect blackOutlineFrame = NSMakeRect(0.0, 0.0, [self bounds].size.width, [self bounds].size.height-1.0);
    double radius = self.bounds.size.height / 2;

    NSGradient *gradient = [[NSGradient alloc] initWithStartingColor:self.startColor endingColor:self.endColor];
    [gradient drawInBezierPath:[NSBezierPath bezierPathWithRoundedRect:blackOutlineFrame xRadius:radius yRadius:radius] angle:90];
    
    NSMutableDictionary *drawStringAttributes = [[NSMutableDictionary alloc] init];
	[drawStringAttributes setValue:[NSColor whiteColor] forKey:NSForegroundColorAttributeName];
    [drawStringAttributes setValue:[NSFont boldSystemFontOfSize:DOCK_TEXTFIELD_FONTSIZE] forKey:NSFontAttributeName];
	NSShadow *stringShadow = [[NSShadow alloc] init];
	[stringShadow setShadowColor:[NSColor blackColor]];
	NSSize shadowSize;
	shadowSize.width = 2;
	shadowSize.height = -2;
	[stringShadow setShadowOffset:shadowSize];
	[stringShadow setShadowBlurRadius:6];
	[drawStringAttributes setValue:stringShadow forKey:NSShadowAttributeName];
	
    NSString *MRString = _textToDisplay;
	NSString *budgetString = [NSString stringWithFormat:@"%@", MRString];
	NSSize stringSize = [budgetString sizeWithAttributes:drawStringAttributes];
	NSPoint centerPoint;
	centerPoint.x = (dirtyRect.size.width / 2) - (stringSize.width / 2);
	centerPoint.y = dirtyRect.size.height / 2 - (stringSize.height / 2) - 2;
	[budgetString drawAtPoint:centerPoint withAttributes:drawStringAttributes];
}

@end
