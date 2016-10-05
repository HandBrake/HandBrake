/* HBToolbarBadgedItem.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License.
 */

#import "HBToolbarBadgedItem.h"

@interface HBToolbarBadgedItem ()

@property (nonatomic) NSImage *primary;
@property (nonatomic) NSImage *cache;

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
    if (_badgeValue.length)
    {
        _cache = nil;
        [super setImage:[self HB_badgeImage:_badgeValue]];
    }
    else
    {
        [super setImage:image];
    }
}

- (void)setBadgeValue:(NSString *)badgeValue
{
    if (![_badgeValue isEqualToString:badgeValue])
    {
        if (badgeValue.length)
        {
            [super setImage:[self HB_badgeImage:badgeValue]];
        }
        else
        {
            [super setImage:_primary];
        }
        _badgeValue = [badgeValue copy];
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

- (CGColorRef)HB_NSColorToCGColor:(NSColor *)color
{
    // CGColor property of NSColor has been added only in 10.8,
    // we need to support 10.7 too.
    NSInteger numberOfComponents = [color numberOfComponents];
    CGFloat components[numberOfComponents];
    CGColorSpaceRef colorSpace = [[color colorSpace] CGColorSpace];
    [color getComponents:(CGFloat *)&components];
    CGColorRef cgColor = CGColorCreate(colorSpace, components);

    return cgColor;
}

- (void)HB_refreshBadge
{
    if (_badgeValue.length)
    {
        _cache = [self HB_renderImage:_primary withBadge:_badgeValue];
        [super setImage:_cache];
    }
}

- (NSImage *)HB_badgeImage:(NSString *)badgeValue
{
    if (![_badgeValue isEqualToString:badgeValue] || _cache == nil)
    {
        _cache = [self HB_renderImage:_primary withBadge:badgeValue];
    }
    return _cache;
}

- (NSImage *)HB_renderImage:(NSImage *)image withBadge:(NSString *)badge
{
    NSMutableParagraphStyle *paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    paragraphStyle.minimumLineHeight = 0.0f;

    NSImage *newImage = [[NSImage alloc] initWithSize:image.size];
    for (NSImageRep *rep in image.representations)
    {
        NSSize size = rep.size;
        NSBitmapImageRep *newRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                                           pixelsWide:(NSInteger)floor(size.width)
                                                                           pixelsHigh:(NSInteger)floor(size.height)
                                                                        bitsPerSample:8
                                                                      samplesPerPixel:4
                                                                             hasAlpha:YES
                                                                             isPlanar:NO
                                                                       colorSpaceName:NSDeviceRGBColorSpace
                                                                          bytesPerRow:(NSInteger)floor(size.width) * 4
                                                                         bitsPerPixel:32];

        NSGraphicsContext *ctx = [NSGraphicsContext graphicsContextWithBitmapImageRep:newRep];
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:ctx];

        CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
        CGContextSaveGState(context);

        NSRect imageRect = NSMakeRect(0, 0, size.width, size.height);
        CGImageRef ref = [image CGImageForProposedRect:&imageRect context:[NSGraphicsContext currentContext] hints:nil];
        CGContextDrawImage(context, imageRect, ref);

        // Work out the area
        CGFloat iconsize = size.width * 0.5f;
        CGFloat radius = iconsize * 0.5f;
        NSPoint indent = NSMakePoint(10, 2);

        NSFont *font = [NSFont boldSystemFontOfSize:10];
        NSDictionary *attr = @{NSParagraphStyleAttributeName : paragraphStyle,
                     NSFontAttributeName : font,
                     NSForegroundColorAttributeName : _badgeTextColor };

        NSRect textSize = [badge boundingRectWithSize:NSZeroSize options:NSStringDrawingOneShot attributes:attr];
        NSRect badgeRect = NSMakeRect(size.width - textSize.size.width - indent.x, size.height - textSize.size.height - indent.y,
                                      textSize.size.width + indent.x, textSize.size.height + indent.y);

        // Draw the ellipse
        CGFloat minx = CGRectGetMinX(badgeRect);
        CGFloat midx = CGRectGetMidX(badgeRect);
        CGFloat maxx = CGRectGetMaxX(badgeRect);
        CGFloat miny = CGRectGetMinY(badgeRect);
        CGFloat midy = CGRectGetMidY(badgeRect);
        CGFloat maxy = CGRectGetMaxY(badgeRect);

        // Fill the ellipse
        CGContextSaveGState(context);
        CGContextBeginPath(context);
        CGContextMoveToPoint(context, minx, midy);
        CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
        CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
        CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
        CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
        CGContextClosePath(context);
        CGContextSetFillColorWithColor(context, [self HB_NSColorToCGColor:_badgeFillColor]);
        CGContextDrawPath(context, kCGPathFill);

        // Draw the text
        NSRect textBounds = [badge boundingRectWithSize:NSZeroSize
                                                options:NSStringDrawingUsesDeviceMetrics
                                             attributes:attr];

        badgeRect.origin.x = CGRectGetMidX(badgeRect) - (textSize.size.width * 0.5f);
        badgeRect.origin.x -= (textBounds.size.width - textSize.size.width) * 0.5f;
        badgeRect.origin.y = CGRectGetMidY(badgeRect);
        badgeRect.origin.y -= textBounds.origin.y;
        badgeRect.origin.y -= ((textBounds.size.height - textSize.origin.y) * 0.5f);

        badgeRect.size.height = textSize.size.height;
        badgeRect.size.width = textSize.size.width;
        [badge drawInRect:badgeRect withAttributes:attr];

        CGContextRestoreGState(context);

        CGContextFlush(context);
        CGContextRestoreGState(context);

        [newImage addRepresentation:newRep];
    }

    return newImage;
}

@end
