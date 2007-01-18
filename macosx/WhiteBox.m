#import "WhiteBox.h"

@implementation WhiteBox

- (void) drawRect: (NSRect) rect
{
    [[NSColor whiteColor] set];
    [[NSBezierPath bezierPathWithRect: rect] fill];
    [super drawRect: rect];
}
    
@end
