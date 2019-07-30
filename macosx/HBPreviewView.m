/* HBPreviewView.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreviewView.h"

// the white border around the preview image
#define BORDER_SIZE 2.0

@interface HBPreviewView ()

@property (nonatomic) CALayer *backLayer;
@property (nonatomic) CALayer *pictureLayer;

@property (nonatomic, readwrite) CGFloat scale;
@property (nonatomic, readwrite) NSRect pictureFrame;


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
    [_backLayer setBounds:CGRectMake(0.0, 0.0, self.frame.size.width, self.frame.size.height)];
    [_backLayer setBackgroundColor: NSColor.whiteColor.CGColor];
    [_backLayer setShadowOpacity:0.5f];
    [_backLayer setShadowOffset:CGSizeMake(0, 0)];
    [_backLayer setAnchorPoint:CGPointMake(0, 0)];
    _backLayer.opaque = YES;

    _pictureLayer = [CALayer layer];
    [_pictureLayer setBounds:CGRectMake(0.0, 0.0, self.frame.size.width - (BORDER_SIZE * 2), self.frame.size.height - (BORDER_SIZE * 2))];
    [_pictureLayer setAnchorPoint:CGPointMake(0, 0)];
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

    _scale = 1;
    _pictureFrame = _pictureLayer.frame;
}

- (void)setImage:(CGImageRef)image
{
    _image = image;
    self.pictureLayer.contents = (__bridge id)(image);

    // Hide the layers if there is no image
    BOOL hidden = _image == nil ? YES : NO;
    self.pictureLayer.hidden = hidden ;
    self.backLayer.hidden = hidden || !self.showBorder;

    [self _updatePreviewLayout];
}

- (void)setFitToView:(BOOL)fitToView
{
    _fitToView = fitToView;
    [self _updatePreviewLayout];
}

- (void)setShowBorder:(BOOL)showBorder
{
    _showBorder = showBorder;
    self.backLayer.hidden = !showBorder;
    [self _updatePreviewLayout];
}

- (void)setShowShadow:(BOOL)showShadow
{
    _backLayer.shadowOpacity = showShadow ? 0.5f : 0;
}

- (void)setFrame:(NSRect)newRect {
    // A change in size has required the view to be invalidated.
    if ([self inLiveResize]) {
        [super setFrame:newRect];
    }
    else {
        [super setFrame:newRect];
    }

    [self _updatePreviewLayout];
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

/**
 *  Updates the sublayers layout.
 */
- (void)_updatePreviewLayout
{
    // Set the picture size display fields below the Preview Picture
    NSSize imageSize = NSMakeSize(CGImageGetWidth(self.image), CGImageGetHeight(self.image));
    CGFloat backingScaleFactor = 1.0;

    if (imageSize.width > 0 && imageSize.height > 0) {

        if (self.window)
        {
            backingScaleFactor = self.window.backingScaleFactor;
        }
        // HiDPI mode usually display everything
        // with douple pixel count, but we don't
        // want to double the size of the video
        NSSize imageScaledSize = NSMakeSize(imageSize.width / backingScaleFactor, imageSize.height / backingScaleFactor);
        NSSize frameSize = self.frame.size;

        if (self.showBorder == YES)
        {
            frameSize.width -= BORDER_SIZE * 2;
            frameSize.height -= BORDER_SIZE * 2;
        }

        if (self.fitToView == YES)
        {
            // We are in Fit to View mode so, we have to get the ratio for height and width against the window
            // size so we can scale from there.
            imageScaledSize = [self scaledSize:imageScaledSize toFit:frameSize];
        }
        else
        {
            // If the image is larger then the view, scale the image
            if (imageScaledSize.width > frameSize.width || imageScaledSize.height > frameSize.height)
            {
                imageScaledSize = [self scaledSize:imageScaledSize toFit:frameSize];
            }
        }

        [NSAnimationContext beginGrouping];
        [[NSAnimationContext currentContext] setDuration:0];

        // Resize the CALayers
        CGRect backRect = CGRectMake(0, 0, imageScaledSize.width + (BORDER_SIZE * 2), imageScaledSize.height + (BORDER_SIZE * 2));
        CGRect pictureRect = CGRectMake(0, 0, imageScaledSize.width, imageScaledSize.height);

        backRect = CGRectIntegral(backRect);
        pictureRect = CGRectIntegral(pictureRect);

        self.backLayer.bounds = backRect;
        self.pictureLayer.bounds = pictureRect;

        // Position the CALayers
        NSRect alignedRect = [self backingAlignedRect:NSMakeRect(0, 0,
                                                                 self.frame.size.width - pictureRect.size.width,
                                                                 self.frame.size.height - pictureRect.size.height)
                                              options:NSAlignAllEdgesNearest];

        CGPoint anchor = CGPointMake(alignedRect.size.width / 2,
                                     alignedRect.size.height / 2);
        [self.pictureLayer setPosition:anchor];

        CGPoint backAchor = CGPointMake(anchor.x - BORDER_SIZE, anchor.y - BORDER_SIZE);
        [self.backLayer setPosition:backAchor];
        
        [NSAnimationContext endGrouping];
        
        // Update the properties
        self.scale = self.pictureLayer.frame.size.width / imageSize.width * backingScaleFactor;
        self.pictureFrame = self.pictureLayer.frame;
    }
}

/**
 * Given the size of the preview image to be shown, returns the best possible
 * size for the view.
 */
- (NSSize)optimalViewSizeForImageSize:(NSSize)imageSize minSize:(NSSize)minSize
{
    if (self.window.backingScaleFactor != 1.0)
    {
        // HiDPI mode usually display everything
        // with douple pixel count, but we don't
        // want to double the size of the video
        imageSize.height /= self.window.backingScaleFactor;
        imageSize.width /= self.window.backingScaleFactor;
    }

    NSSize screenSize = self.window.screen.visibleFrame.size;
    CGFloat maxWidth = screenSize.width;
    CGFloat maxHeight = screenSize.height;

    NSSize resultSize = imageSize;

    if (resultSize.width > maxWidth || resultSize.height > maxHeight)
    {
        resultSize = [self scaledSize:resultSize toFit:screenSize];
    }

    // If necessary, grow to minimum dimensions to ensure controls overlay is not obstructed
    if (resultSize.width < minSize.width)
    {
        resultSize.width = minSize.width;
    }
    if (resultSize.height < minSize.height)
    {
        resultSize.height = minSize.height;
    }

    // Add the border
    if (self.showBorder)
    {
        resultSize.width += BORDER_SIZE * 2;
        resultSize.height += BORDER_SIZE * 2;
    }

    NSRect alignedRect = [self backingAlignedRect:NSMakeRect(0, 0, resultSize.width, resultSize.height)
                                          options:NSAlignAllEdgesNearest];

    resultSize.width = alignedRect.size.width;
    resultSize.height = alignedRect.size.height;

    return resultSize;
}

#pragma MARK: - Accessibility

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
