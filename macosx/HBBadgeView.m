/*  HBBadgeView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBBadgeView.h"

@implementation HBBadgeView

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
    {
        _badgeValue = @"0";
        _badgeFillColor = [NSColor redColor];
        _badgeTextColor = [NSColor whiteColor];
    }
    return self;
}

- (nullable NSView *)hitTest:(NSPoint)point
{
    return nil;
}

- (void)setBadgeValue:(NSString *)badgeValue
{
    _badgeValue = [badgeValue copy];
    [self setNeedsDisplay:YES];
}

- (void)setBadgeTextColor:(NSColor *)badgeTextColor
{
    _badgeTextColor = [badgeTextColor copy];
    [self setNeedsDisplay:YES];
}

- (void)setBadgeFillColor:(NSColor *)badgeFillColor
{
    _badgeFillColor = [badgeFillColor copy];
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];

    if (_badgeValue.length == 0)
    {
        return;
    }

    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    CGContextSaveGState(context);

    CGFloat pointSize = 9;

    NSMutableParagraphStyle *paragraphStyle = [NSParagraphStyle.defaultParagraphStyle mutableCopy];
    NSFont *font = [NSFont boldSystemFontOfSize:pointSize];
    NSDictionary *attr = @{NSParagraphStyleAttributeName: paragraphStyle,
                           NSFontAttributeName: font,
                           NSForegroundColorAttributeName: _badgeTextColor};

    NSRect textBounds = [_badgeValue boundingRectWithSize:NSZeroSize options:NSStringDrawingUsesFontLeading attributes:attr context:nil];

    NSPoint indent = NSMakePoint(pointSize, 2);
    CGFloat radius = (textBounds.size.height + indent.y) * 0.5f;

    NSRect badgeRect = NSMakeRect(0, 0, textBounds.size.width + indent.x, textBounds.size.height + indent.y);
    badgeRect = NSIntegralRectWithOptions(badgeRect, NSAlignAllEdgesOutward);

    NSSize size = self.bounds.size;
    if (@available(macOS 14.0, *))
    {
        badgeRect.origin.x = 25;
        badgeRect.origin.y = 2;
    }
    else
    {
        badgeRect.origin.x = size.width - badgeRect.size.width;
    }

    // Draw the ellipse
    CGFloat minx = CGRectGetMinX(badgeRect);
    CGFloat midx = CGRectGetMidX(badgeRect);
    CGFloat maxx = CGRectGetMaxX(badgeRect);
    CGFloat miny = CGRectGetMinY(badgeRect);
    CGFloat midy = CGRectGetMidY(badgeRect);
    CGFloat maxy = CGRectGetMaxY(badgeRect);

    // Fill the ellipse
    CGContextBeginPath(context);
    CGContextMoveToPoint(context, minx, midy);
    CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
    CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
    CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
    CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
    CGContextClosePath(context);
    CGColorRef fillColor = _badgeFillColor.CGColor;
    CGContextSetFillColorWithColor(context,fillColor);
    CGContextDrawPath(context, kCGPathFill);

    // Draw the text
    badgeRect.origin.x = CGRectGetMidX(badgeRect);
    badgeRect.origin.x -= textBounds.origin.x / 2;
    badgeRect.origin.x -= ((textBounds.size.width - textBounds.origin.x) * 0.5f);
    badgeRect.origin.y = CGRectGetMidY(badgeRect);
    badgeRect.origin.y -= textBounds.origin.y / 2;
    badgeRect.origin.y -= ((textBounds.size.height - textBounds.origin.y) * 0.5f);

    badgeRect.origin.x = floor(badgeRect.origin.x) + 0.5;
    badgeRect.origin.y = ceil(badgeRect.origin.y) + 0.5;
    badgeRect.size.width = textBounds.size.width;
    badgeRect.size.height = textBounds.size.height;

    [_badgeValue drawInRect:badgeRect withAttributes:attr];

    CGContextRestoreGState(context);
}

@end
