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

- (CGColorRef)copyNSColorToCGColor:(NSColor *)color
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

- (NSImage *)HB_renderImage:(NSImage *)image withBadge:(NSString *)badgeString
{
    NSMutableParagraphStyle *paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];

    NSImage *newImage = [[NSImage alloc] initWithSize:image.size];
    for (NSImageRep *rep in image.representations)
    {
        NSSize size = NSMakeSize(rep.pixelsWide, rep.pixelsHigh);
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

        CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
        CGContextSaveGState(context);

        NSRect imageRect = NSMakeRect(0, 0, size.width, size.height);
        CGImageRef ref = [image CGImageForProposedRect:&imageRect context:[NSGraphicsContext currentContext] hints:nil];
        CGContextDrawImage(context, imageRect, ref);

        // Work out the area
        CGFloat scaleFactor = rep.pixelsWide / rep.size.width;
        CGFloat typeSize = 10;
        if (scaleFactor > 1)
        {
            typeSize = 8;
        }
        CGFloat pointSize = typeSize * scaleFactor;

        NSFont *font = [NSFont boldSystemFontOfSize:pointSize];
        NSDictionary *attr = @{NSParagraphStyleAttributeName : paragraphStyle,
                               NSFontAttributeName : font,
                               NSForegroundColorAttributeName : _badgeTextColor };

        NSRect textBounds = [badgeString boundingRectWithSize:NSZeroSize
                                                      options:0
                                                   attributes:attr];

        NSPoint indent = NSMakePoint(typeSize * scaleFactor, 2 * scaleFactor);
        CGFloat radius = (textBounds.size.height + indent.y) * 0.5f;

        CGFloat offset_x = 0;
        CGFloat offset_y = 0;
        if (scaleFactor > 1)
        {
            offset_y = 2 * scaleFactor;
        }
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
        CGContextSaveGState(context);
        CGContextBeginPath(context);
        CGContextMoveToPoint(context, minx, midy);
        CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
        CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
        CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
        CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
        CGContextClosePath(context);
        CGColorRef fillColor = [self copyNSColorToCGColor:_badgeFillColor];
        CGContextSetFillColorWithColor(context,fillColor);
        CFRelease(fillColor);
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

        [badgeString drawInRect:badgeRect withAttributes:attr];

        CGContextRestoreGState(context);

        CGContextFlush(context);
        CGContextRestoreGState(context);

        [NSGraphicsContext restoreGraphicsState];

        [newImage addRepresentation:newRep];
    }

    return newImage;
}

@end
