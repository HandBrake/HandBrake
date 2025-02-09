/* HBPreviewView.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreviewView.h"

@import AVFoundation;

// the white border around the preview image
#define BORDER_SIZE 2.0

@interface HBPreviewView ()

@property (nonatomic) CALayer *backLayer;
@property (nonatomic) CALayer *imageLayer;
@property (nonatomic) AVSampleBufferDisplayLayer *displayLayer;

@property (nonatomic) NSSize size;

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

    [self initBackground];
    [self initImageLayer];

    _imageLayer.hidden = YES;
    _backLayer.hidden = YES;
    _showBorder = YES;
}

- (void)initBackground
{
    _backLayer = [CALayer layer];
    _backLayer.bounds = CGRectMake(0.0, 0.0, self.frame.size.width, self.frame.size.height);
    _backLayer.backgroundColor = NSColor.whiteColor.CGColor;
    _backLayer.shadowOpacity = 0.5f;
    _backLayer.shadowOffset = CGSizeZero;
    _backLayer.anchorPoint = CGPointZero;
    _backLayer.opaque = YES;

    [self.layer addSublayer:_backLayer];
}

- (void)initImageLayer
{
    _imageLayer = [CALayer layer];
    _imageLayer.bounds = CGRectMake(0.0, 0.0, self.frame.size.width - (BORDER_SIZE * 2), self.frame.size.height - (BORDER_SIZE * 2));
    _imageLayer.anchorPoint = CGPointZero;
    _imageLayer.opaque = YES;

    // Disable fade on contents change.
    NSMutableDictionary *actions = [NSMutableDictionary dictionary];
    if (_imageLayer.actions)
    {
        [actions addEntriesFromDictionary:_imageLayer.actions];
    }

    actions[@"contents"] = [NSNull null];
    _imageLayer.actions = actions;

    [self.layer addSublayer:_imageLayer];
}

- (void)initDisplayLayer
{
    _displayLayer = [AVSampleBufferDisplayLayer layer];
    _displayLayer.bounds = CGRectMake(0.0, 0.0, self.frame.size.width - (BORDER_SIZE * 2), self.frame.size.height - (BORDER_SIZE * 2));
    _displayLayer.anchorPoint = CGPointZero;
    _displayLayer.opaque = YES;
    _displayLayer.videoGravity = AVLayerVideoGravityResize;

    if (@available(macOS 14.0, *))
    {
        _displayLayer.wantsExtendedDynamicRangeContent = NO;
        _displayLayer.preventsDisplaySleepDuringVideoPlayback = NO;
    }

    [self.layer addSublayer:_displayLayer];
}

- (void)removeDisplayLayer
{
    if (self.displayLayer)
    {
        if (@available(macOS 14.0, *))
        {
            [self.displayLayer.sampleBufferRenderer flushWithRemovalOfDisplayedImage:YES completionHandler:NULL];
        }
        [self.displayLayer removeFromSuperlayer];
        self.displayLayer = nil;
    }
}

- (void)viewDidChangeBackingProperties
{
    if (self.window)
    {
        self.needsLayout = YES;
    }
}

- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer
{
    if (@available(macOS 14.0, *))
    {
        self.imageLayer.contents = nil;

        if (self.displayLayer == nil)
        {
            [self initDisplayLayer];
        }

        // Read the display size
        CFDictionaryRef displaySize = CVBufferCopyAttachment(pixelBuffer, kCVImageBufferDisplayDimensionsKey, NULL);

        CFNumberRef displayWidthNum  = CFDictionaryGetValue(displaySize, kCVImageBufferDisplayWidthKey);
        CFNumberRef displayHeightNum = CFDictionaryGetValue(displaySize, kCVImageBufferDisplayHeightKey);

        int displayWidth, displayHeight;

        CFNumberGetValue(displayWidthNum, kCFNumberSInt32Type, &displayWidth);
        CFNumberGetValue(displayHeightNum, kCFNumberSInt32Type, &displayHeight);

        CFRelease(displaySize);

        // Wrap the pixel buffer in a sample buffer
        CMSampleBufferRef sampleBuffer = NULL;
        CMVideoFormatDescriptionRef formatDescription = NULL;

        CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer, &formatDescription);
        CMSampleTimingInfo timingInfo = {kCMTimeInvalid, kCMTimeZero, kCMTimeInvalid};

        CMSampleBufferCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer, 1, NULL, NULL,
                                           formatDescription,
                                           &timingInfo,
                                           &sampleBuffer);

        CFArrayRef attachmentsArray = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, 1);
        if (attachmentsArray && CFArrayGetCount(attachmentsArray) > 0)
        {
            CFMutableDictionaryRef dict = (CFMutableDictionaryRef)CFArrayGetValueAtIndex(attachmentsArray, 0);
            if (dict)
            {
                CFDictionarySetValue(dict, kCMSampleAttachmentKey_DisplayImmediately, kCFBooleanTrue);
            }
        }

        // Display the sample buffer
        [self.displayLayer.sampleBufferRenderer enqueueSampleBuffer:sampleBuffer];
        self.size = NSMakeSize(displayWidth, displayHeight);

        CFRelease(sampleBuffer);
        CFRelease(formatDescription);
    }
}

- (void)displayCGImage:(CGImageRef)image
{
    [self removeDisplayLayer];

    self.imageLayer.contents = (__bridge id)image;
    self.size = NSMakeSize(CGImageGetWidth(image), CGImageGetHeight(image));
}

- (void)displayImage:(NSImage *)image
{
    [self removeDisplayLayer];

    self.imageLayer.contents = image;
    self.size = image.size;
}

- (void)setImage:(id)image
{
    if (image)
    {
        CFTypeID type = CFGetTypeID((__bridge CFTypeRef)(image));

        if (type == CVPixelBufferGetTypeID())
        {
            [self displayPixelBuffer:(__bridge CVPixelBufferRef)(image)];
        }
        else if (type == CGImageGetTypeID())
        {
            [self displayCGImage:(__bridge CGImageRef)(image)];
        }
        else if ([image isKindOfClass:[NSImage class]])
        {
            [self displayImage:image];
        }
    }
    else
    {
        [self removeDisplayLayer];
        self.imageLayer.contents = nil;
    }

    // Hide the layers if there is no image
    BOOL hidden = self.size.width == 0 ? YES : NO;
    self.imageLayer.hidden = hidden;
    self.displayLayer.hidden = hidden;
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
    if (self.size.width > 0 && self.size.height > 0)
    {
        CGFloat backingScaleFactor = self.window.backingScaleFactor;
        CGFloat borderSize = self.showBorder ? BORDER_SIZE : 0;

        NSSize imageScaledSize = [self imageScaledSize:self.size toFit:self.frame.size
                                            borderSize:borderSize scaleFactor:self.window.backingScaleFactor];

        return (imageScaledSize.width - borderSize * 2) / self.size.width * backingScaleFactor;
    }
    else
    {
        return 1;
    }
}

- (CGRect)pictureFrame
{
    return self.imageLayer.frame;
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

    if (self.size.width > 0 && self.size.height > 0)
    {
        CGFloat borderSize = self.showBorder ? BORDER_SIZE : 0;
        NSSize frameSize = self.frame.size;

        NSSize imageScaledSize = [self imageScaledSize:self.size
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
        self.imageLayer.frame = NSInsetRect(alignedRect, borderSize, borderSize);
        self.displayLayer.frame = NSInsetRect(alignedRect, borderSize, borderSize);

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
    if (self.size.width && self.size.height)
    {
        return [NSString stringWithFormat:NSLocalizedString(@"Preview Image, Size: %f x %f, Scale: %.0f%%", @"Preview -> accessibility label"), self.size.width, self.size.height, self.scale * 100];
    }
    return NSLocalizedString(@"No image", @"Preview -> accessibility label");
}

@end
