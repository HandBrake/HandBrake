/* HBToolbarBadgedItem.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License.
 */

#import "HBToolbarBadgedItem.h"

@interface HBToolbarBadgedItem ()

@property (nonatomic) NSImage *primary;
@property (nonatomic) NSImage *badged;

@end

@implementation HBToolbarBadgedItem

- (instancetype)initWithItemIdentifier:(NSString *)itemIdentifier
{
    self = [super initWithItemIdentifier:itemIdentifier];
    if (self)
    {
        _badgeFillColor = [NSColor redColor];
        _badgeTextColor = [NSColor whiteColor];
    }
    return self;
}

- (void)awakeFromNib
{
    if ([self respondsToSelector:@selector(awakeFromNib)])
    {
        [super awakeFromNib];
    }

    [self HB_refreshBadge];
}

#pragma mark - Public

- (void)setImage:(NSImage *)image
{
    _primary = image;
    [self HB_refreshBadge];
}

- (void)setBadgeValue:(NSString *)badgeValue
{
    if (![_badgeValue isEqualToString:badgeValue])
    {
        _badgeValue = [badgeValue copy];
        [self HB_refreshBadge];
    }
}

- (void)setBadgeTextColor:(NSColor *)badgeTextColor
{
    _badgeTextColor = [badgeTextColor copy];
    [self HB_refreshBadge];
}

- (void)setBadgeFillColor:(NSColor *)badgeFillColor
{
    _badgeFillColor = [badgeFillColor copy];
    [self HB_refreshBadge];
}

#pragma mark -- Private Methods

- (void)HB_refreshBadge
{
    if (_badgeValue.length && _primary)
    {
        __weak HBToolbarBadgedItem *weakSelf = self;
        _badged = [NSImage imageWithSize:_primary.size flipped:NO drawingHandler:^BOOL(NSRect dstRect) {
            HBToolbarBadgedItem *strongSelf = weakSelf;
            if (strongSelf == nil)
            {
                return NO;
            }

            CGContextRef context = NSGraphicsContext.currentContext.CGContext;
            CGContextSaveGState(context);

            CGRect deviceRect = CGContextConvertRectToDeviceSpace(context, dstRect);
            NSSize size = dstRect.size;

            NSRect imageRect = NSMakeRect(0, 0, strongSelf->_primary.size.width, strongSelf->_primary.size.height);
            CGImageRef ref = [strongSelf->_primary CGImageForProposedRect:&imageRect context:NSGraphicsContext.currentContext hints:nil];
            CGContextDrawImage(context, imageRect, ref);

            // Work out the area
            CGFloat scaleFactor = deviceRect.size.width / strongSelf->_primary.size.width;
            CGFloat typeSize = 10;
            if (scaleFactor > 1)
            {
                typeSize = 8;
            }
            CGFloat pointSize = typeSize;

            NSMutableParagraphStyle *paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
            NSFont *font = [NSFont boldSystemFontOfSize:pointSize];
            NSDictionary *attr = @{NSParagraphStyleAttributeName : paragraphStyle,
                                   NSFontAttributeName : font,
                                   NSForegroundColorAttributeName : strongSelf->_badgeTextColor };

            NSRect textBounds = [strongSelf->_badgeValue boundingRectWithSize:NSZeroSize
                                                                      options:0
                                                                   attributes:attr];

            NSPoint indent = NSMakePoint(typeSize, 2);
            CGFloat radius = (textBounds.size.height + indent.y) * 0.5f;

            CGFloat offset_x = 0;
            CGFloat offset_y = 0;
            NSRect badgeRect = NSMakeRect(size.width - textBounds.size.width - indent.x - offset_x, offset_y,
                                          textBounds.size.width + indent.x, textBounds.size.height + indent.y);
            badgeRect = NSIntegralRect(badgeRect);

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
            CGColorRef fillColor = strongSelf->_badgeFillColor.CGColor;
            CGContextSetFillColorWithColor(context,fillColor);
            CGContextDrawPath(context, kCGPathFill);

            // Draw the text
            badgeRect.origin.x = CGRectGetMidX(badgeRect);
            badgeRect.origin.x -= textBounds.origin.x / 2;
            badgeRect.origin.x -= ((textBounds.size.width - textBounds.origin.x) * 0.5f);
            badgeRect.origin.y = CGRectGetMidY(badgeRect);
            badgeRect.origin.y -= textBounds.origin.y / 2;
            badgeRect.origin.y -= ((textBounds.size.height - textBounds.origin.y) * 0.5f);

            badgeRect.origin.x = floor(badgeRect.origin.x);
            badgeRect.origin.y = floor(badgeRect.origin.y);
            badgeRect.size.width = textBounds.size.width;
            badgeRect.size.height = textBounds.size.height;

            [strongSelf->_badgeValue drawInRect:badgeRect withAttributes:attr];

            CGContextRestoreGState(context);

            return YES;
        }];
        [super setImage:_badged];
    }
    else
    {
        [super setImage:_primary];
    }
}

@end
