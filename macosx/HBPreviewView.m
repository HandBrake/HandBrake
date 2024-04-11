/* HBPreviewView.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreviewView.h"
#import <QuartzCore/QuartzCore.h>

// the white border around the preview image
#define BORDER_SIZE 2.0

@interface HBPreviewView ()

@property (nonatomic) CALayer *backLayer;
@property (nonatomic) CALayer *pictureLayer;

@end

@implementation HBPreviewView

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];

    if (self)
    {
        [self setUp];
    }

    return self;
}

- (nullable instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];

    if (self)
    {
        [self setUp];
    }

    return self;
}

- (BOOL)clipsToBounds
{
    return YES;
}

/**
 *  Setups the sublayers,
 *  called by every initializer.
 */
- (void)setUp
{
    // Make it a layer hosting view
    self.layer = [CALayer new];
    self.wantsLayer = YES;

    _backLayer = [CALayer layer];
    _backLayer.bounds = CGRectMake(0.0, 0.0, self.frame.size.width, self.frame.size.height);
    _backLayer.backgroundColor = NSColor.whiteColor.CGColor;
    _backLayer.shadowOpacity = 0.5f;
    _backLayer.shadowOffset = CGSizeZero;
    _backLayer.anchorPoint = CGPointZero;
    _backLayer.opaque = YES;

    _pictureLayer = [CALayer layer];
    _pictureLayer.bounds = CGRectMake(0.0, 0.0, self.frame.size.width - (BORDER_SIZE * 2), self.frame.size.height - (BORDER_SIZE * 2));
    _pictureLayer.anchorPoint = CGPointZero;
    _pictureLayer.opaque = YES;

    // Disable fade on contents change.
    NSMutableDictionary *actions = [NSMutableDictionary dictionary];
    if (_pictureLayer.actions)
    {
        [actions addEntriesFromDictionary:_pictureLayer.actions];
    }

    actions[@"contents"] = [NSNull null];
    _pictureLayer.actions = actions;

    [self.layer addSublayer:_backLayer];
    [self.layer addSublayer:_pictureLayer];

    _pictureLayer.hidden = YES;
    _backLayer.hidden = YES;
    _showBorder = YES;
}

- (void)viewDidChangeBackingProperties
{
    if (self.window)
    {
        self.needsLayout = YES;
    }
}

- (void)setImage:(CGImageRef)image
{
    _image = image;
    self.pictureLayer.contents = (__bridge id)(image);

    // Hide the layers if there is no image
    BOOL hidden = _image == nil ? YES : NO;
    self.pictureLayer.hidden = hidden ;
    self.backLayer.hidden = hidden || !self.showBorder;

    self.needsLayout = YES;
}

- (void)setFitToView:(BOOL)fitToView
{
    _fitToView = fitToView;
    self.needsLayout = YES;
}

- (void)setShowBorder:(BOOL)showBorder
{
    _showBorder = showBorder;
    self.backLayer.hidden = !showBorder;
    self.needsLayout = YES;
}

- (void)setShowShadow:(BOOL)showShadow
{
    _backLayer.shadowOpacity = showShadow ? 0.5f : 0;
}

- (CGFloat)scale
{
    if (self.image)
    {
        NSSize imageSize = NSMakeSize(CGImageGetWidth(self.image), CGImageGetHeight(self.image));
        CGFloat backingScaleFactor = self.window.backingScaleFactor;
        CGFloat borderSize = self.showBorder ? BORDER_SIZE : 0;

        NSSize imageScaledSize = [self imageScaledSize:imageSize toFit:self.frame.size borderSize:borderSize scaleFactor:self.window.backingScaleFactor];

        return (imageScaledSize.width - borderSize * 2) / imageSize.width * backingScaleFactor;
    }
    else
    {
        return 1;
    }
}

- (CGRect)pictureFrame
{
    return self.pictureLayer.frame;
}

- (NSSize)scaledSize:(NSSize)source toFit:(NSSize)destination
{
    NSSize result;
    CGFloat sourceAspectRatio = source.width / source.height;
    CGFloat destinationAspectRatio = destination.width / destination.height;

    // Source is larger than screen in one or more dimensions
    if (sourceAspectRatio > destinationAspectRatio)
    {
        // Source aspect wider than screen aspect, snap to max width and vary height
        result.width = destination.width;
        result.height = result.width / sourceAspectRatio;
    }
    else
    {
        // Source aspect narrower than screen aspect, snap to max height vary width
        result.height = destination.height;
        result.width = result.height * sourceAspectRatio;
    }

    return result;
}

- (NSSize)imageScaledSize:(NSSize)source toFit:(NSSize)destination borderSize:(CGFloat)borderSize scaleFactor:(CGFloat)scaleFactor
{
    // HiDPI mode usually display everything
    // with double pixel count, but we don't
    // want to double the size of the video
    NSSize scaledSource = NSMakeSize(source.width / scaleFactor, source.height / scaleFactor);

    scaledSource.width += borderSize * 2;
    scaledSource.height += borderSize * 2;

    if (self.fitToView == YES || scaledSource.width > destination.width || scaledSource.height > destination.height)
    {
        // If the image is larger then the view or if we are in Fit to View mode, scale the image
        scaledSource = [self scaledSize:source toFit:destination];
    }

    return scaledSource;
}

- (void)layout
{
    [super layout];

    NSSize imageSize = NSMakeSize(CGImageGetWidth(self.image), CGImageGetHeight(self.image));

    if (imageSize.width > 0 && imageSize.height > 0)
    {
        CGFloat borderSize = self.showBorder ? BORDER_SIZE : 0;
        NSSize frameSize = self.frame.size;

        NSSize imageScaledSize = [self imageScaledSize:imageSize
                                                 toFit:frameSize
                                            borderSize:borderSize
                                           scaleFactor:self.window.backingScaleFactor];

        [CATransaction begin];
        CATransaction.disableActions = YES;

        CGFloat width = imageScaledSize.width;
        CGFloat height = imageScaledSize.height;

        CGFloat offsetX = (frameSize.width - width) / 2;
        CGFloat offsetY = (frameSize.height - height) / 2;

        NSRect alignedRect = [self backingAlignedRect:NSMakeRect(offsetX, offsetY, width, height) options:NSAlignAllEdgesNearest];

        self.backLayer.frame = alignedRect;
        self.pictureLayer.frame = NSInsetRect(alignedRect, borderSize, borderSize);

        [CATransaction commit];
    }
}

/**
 * Given the size of the preview image to be shown,
 *  returns the best possible size for the view.
 */
- (NSSize)optimalViewSizeForImageSize:(NSSize)imageSize minSize:(NSSize)minSize scaleFactor:(CGFloat)scaleFactor
{
    NSSize resultSize = [self imageScaledSize:imageSize
                                        toFit:self.window.screen.visibleFrame.size
                                   borderSize:self.showBorder ? BORDER_SIZE : 0
                                  scaleFactor:scaleFactor];

    // If necessary, grow to minimum dimensions to ensure controls overlay is not obstructed
    if (resultSize.width < minSize.width)
    {
        resultSize.width = minSize.width;
    }
    if (resultSize.height < minSize.height)
    {
        resultSize.height = minSize.height;
    }

    NSRect alignedRect = [self backingAlignedRect:NSMakeRect(0, 0, resultSize.width, resultSize.height)
                                          options:NSAlignAllEdgesNearest];

    return alignedRect.size;
}

#pragma mark - Accessibility

- (BOOL)isAccessibilityElement
{
    return YES;
}

- (NSString *)accessibilityRole
{
    return NSAccessibilityImageRole;
}

- (NSString *)accessibilityLabel
{
    if (self.image)
    {
        return [NSString stringWithFormat:NSLocalizedString(@"Preview Image, Size: %zu x %zu, Scale: %.0f%%", @"Preview -> accessibility label"), CGImageGetWidth(self.image), CGImageGetHeight(self.image), self.scale * 100];
    }
    return NSLocalizedString(@"No image", @"Preview -> accessibility label");
}

@end
