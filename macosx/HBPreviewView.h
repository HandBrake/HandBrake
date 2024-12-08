/* HBPreviewView.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/**
 *  A HBPreviewView is a subclass of NSView that can be used to display an image
 *  plus a border.
 */
@interface HBPreviewView : NSView

/**
 *  The image displayed by the view.
 */
@property (nonatomic, readwrite, nullable) id image;

/**
 *  The scale at which the image is shown.
 */
@property (nonatomic, readonly) CGFloat scale;

/**
 *  The actual frame of the displayed image.
 */
@property (nonatomic, readonly) CGRect pictureFrame;

/**
 *  Whether the image will be scaled to fill the view
 *  or not.
 */
@property (nonatomic, readwrite) BOOL fitToView;

/**
 *  If enabled, the view will show a white border around the image.
 */
@property (nonatomic, readwrite) BOOL showBorder;

/**
 *  If enabled, the view will show a shadow around the image.
 */
@property (nonatomic, readwrite) BOOL showShadow;

/**
 * Given the size of the preview image to be shown, returns the best possible
 * size for the view.
 */
- (NSSize)optimalViewSizeForImageSize:(NSSize)imageSize minSize:(NSSize)minSize scaleFactor:(CGFloat)scaleFactor;

@end

NS_ASSUME_NONNULL_END
