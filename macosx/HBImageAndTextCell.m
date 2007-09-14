/* HBImageAndTextCell

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.m0k.org/>.
    It may be used under the terms of the GNU General Public License.
*/


#import "HBImageAndTextCell.h"


static inline float
xLeftInRect(NSSize innerSize, NSRect outerRect)
{
  return NSMinX(outerRect);
}

static inline float
xCenterInRect(NSSize innerSize, NSRect outerRect)
{
  return MAX(NSMidX(outerRect) - (innerSize.width/2.0), 0.0);
}

static inline float
xRightInRect(NSSize innerSize, NSRect outerRect)
{
  return MAX(NSMaxX(outerRect) - innerSize.width, 0.0);
}

static inline float
yTopInRect(NSSize innerSize, NSRect outerRect, BOOL flipped)
{
  if (flipped)
    return NSMinY(outerRect);
  else
    return MAX(NSMaxY(outerRect) - innerSize.height, 0.0);
}

static inline float
yCenterInRect(NSSize innerSize, NSRect outerRect, BOOL flipped)
{
  return MAX(NSMidY(outerRect) - innerSize.height/2.0, 0.0);
}

static inline float
yBottomInRect(NSSize innerSize, NSRect outerRect, BOOL flipped)
{
  if (flipped)
    return MAX(NSMaxY(outerRect) - innerSize.height, 0.0);
  else
    return NSMinY(outerRect);
}

static inline NSSize
scaleProportionally(NSSize imageSize, NSRect canvasRect)
{
  float ratio;

  // get the smaller ratio and scale the image size by it
  ratio = MIN(NSWidth(canvasRect) / imageSize.width,
	      NSHeight(canvasRect) / imageSize.height);

  imageSize.width *= ratio;
  imageSize.height *= ratio;

  return imageSize;
}



@implementation HBImageAndTextCell

-(id)initTextCell:(NSString *)aString
{
    if (self = [super initTextCell:aString])
    {
        imageAlignment = NSImageAlignTop;
        imageSpacing = NSMakeSize (3.0, 2.0);
    }
    return self; 
}

-(id)initWithCoder:(NSCoder *)decoder
{
    if (self = [super initWithCoder:decoder])
    {
        imageAlignment = NSImageAlignTop;
        imageSpacing = NSMakeSize (3.0, 2.0);
    }
    return self; 
}

- (void)dealloc
{
    [image release];
    image = nil;
    [super dealloc];
}

- copyWithZone:(NSZone *)zone
{
    HBImageAndTextCell *cell = (HBImageAndTextCell *)[super copyWithZone:zone];
    cell->image = [image retain];
    return cell;
}

- (void)setImage:(NSImage *)anImage
{
    if (anImage != image)
    {
        [image release];
        image = [anImage retain];
    }
}

- (NSImage *)image
{
    return image;
}

- (void) setImageAlignment:(NSImageAlignment)alignment;
{
    imageAlignment = alignment;
}

- (NSImageAlignment) imageAlignment;
{
    return imageAlignment;
}

- (void)setImageSpacing:(NSSize)aSize;
{
    imageSpacing = aSize;
}

- (NSSize)imageSpacing
{
    return imageSpacing;
}

- (void)editWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject event:(NSEvent *)theEvent
{
    NSRect textFrame, imageFrame;
    NSDivideRect (aRect, &imageFrame, &textFrame, (imageSpacing.width * 2) + [image size].width, NSMinXEdge);
    [super editWithFrame: textFrame inView: controlView editor:textObj delegate:anObject event: theEvent];
}

- (void)selectWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject start:(int)selStart length:(int)selLength
{
    NSRect textFrame, imageFrame;
    NSDivideRect (aRect, &imageFrame, &textFrame, (imageSpacing.width * 2) + [image size].width, NSMinXEdge);
    [super selectWithFrame: textFrame inView: controlView editor:textObj delegate:anObject start:selStart length:selLength];
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
#if 1
    if (image != nil)
    {
        NSSize imageSize;
        NSRect imageFrame;

        imageSize = [image size];
        NSDivideRect(cellFrame, &imageFrame, &cellFrame, (imageSpacing.width * 2) + imageSize.width, NSMinXEdge);
        if ([self drawsBackground])
        {
            [[self backgroundColor] set];
            NSRectFill(imageFrame);
        }
        imageFrame.origin.x += imageSpacing.width;
        imageFrame.size = imageSize;

        switch (imageAlignment)
        {
            default:
            case NSImageAlignTop:
                if ([controlView isFlipped])
                    imageFrame.origin.y += imageFrame.size.height;
                else
                    imageFrame.origin.y += (cellFrame.size.height - imageFrame.size.height);
                break;

            case NSImageAlignCenter:
                if ([controlView isFlipped])
                    imageFrame.origin.y += ceil((cellFrame.size.height + imageFrame.size.height) / 2);
                else
                    imageFrame.origin.y += ceil((cellFrame.size.height - imageFrame.size.height) / 2);
                break;

             case NSImageAlignBottom:
                if ([controlView isFlipped])
                    imageFrame.origin.y += cellFrame.size.height;
                // for unflipped, imageFrame is already correct
                break;
          
        }
        
        [image compositeToPoint:imageFrame.origin operation:NSCompositeSourceOver];
    }

    [super drawWithFrame:cellFrame inView:controlView];
#endif


#if 0 // this snippet supports all alignment values plus potentially scaling.
    if (image != nil)
    {
        NSSize imageSize;
        NSSize srcImageSize;
        NSRect imageFrame;
        NSPoint	position;
        BOOL flipped = [controlView isFlipped];

        imageSize = [image size];
        srcImageSize = imageSize;   // this will be more useful once/if we support scaling
        
        NSDivideRect(cellFrame, &imageFrame, &cellFrame, 12 + imageSize.width, NSMinXEdge);
        if ([self drawsBackground])
        {
            [[self backgroundColor] set];
            NSRectFill(imageFrame);
        }
        
        switch (imageAlignment)
        {
            default:
            case NSImageAlignLeft:
                position.x = xLeftInRect(imageSize, imageFrame);
                position.y = yCenterInRect(imageSize, imageFrame, flipped);
                break;
            case NSImageAlignRight:
                position.x = xRightInRect(imageSize, imageFrame);
                position.y = yCenterInRect(imageSize, imageFrame, flipped);
                break;
            case NSImageAlignCenter:
                position.x = xCenterInRect(imageSize, imageFrame);
                position.y = yCenterInRect(imageSize, imageFrame, flipped);
                break;
            case NSImageAlignTop:
                position.x = xCenterInRect(imageSize, imageFrame);
                position.y = yTopInRect(imageSize, imageFrame, flipped);
                break;
            case NSImageAlignBottom:
                position.x = xCenterInRect(imageSize, imageFrame);
                position.y = yBottomInRect(imageSize, imageFrame, flipped);
                break;
            case NSImageAlignTopLeft:
                position.x = xLeftInRect(imageSize, imageFrame);
                position.y = yTopInRect(imageSize, imageFrame, flipped);
                break;
            case NSImageAlignTopRight:
                position.x = xRightInRect(imageSize, imageFrame);
                position.y = yTopInRect(imageSize, imageFrame, flipped);
                break;
            case NSImageAlignBottomLeft:
                position.x = xLeftInRect(imageSize, imageFrame);
                position.y = yBottomInRect(imageSize, imageFrame, flipped);
                break;
            case NSImageAlignBottomRight:
                position.x = xRightInRect(imageSize, imageFrame);
                position.y = yBottomInRect(imageSize, imageFrame, flipped);
                break;
        }

        // account for flipped views
        if (flipped)
        {
            position.y += imageSize.height;
            imageSize.height = -imageSize.height;
        }

        // Set image flipping to match view. Don't know if this is really the best way
        // to deal with flipped views and images.
        if ([image isFlipped] != flipped)
            [image setFlipped: flipped];

        // draw!
        [image drawInRect: NSMakeRect(position.x, position.y, imageSize.width, imageSize.height)
            fromRect: NSMakeRect(0, 0, srcImageSize.width,
            srcImageSize.height)
            operation: NSCompositeSourceOver
            fraction: 1.0];

    }

    [super drawWithFrame:cellFrame inView:controlView];
#endif
}

- (NSSize)cellSize
{
    NSSize cellSize = [super cellSize];
    cellSize.width += (image ? [image size].width + (imageSpacing.width * 2) : 0);
    return cellSize;
}

@end

