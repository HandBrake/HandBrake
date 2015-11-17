/* HBPreviewView.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/**
 *  A HBPreviewView is a sublcass of NSView that can be used to display an image
 *  plus a border.
 */
@interface HBPreviewView : NSView

/**
 *  The image displayed by the view.
 */
@property (nonatomic, readwrite, nullable) CGImageRef image;

/**
 *  The scale at which the image is shown.
 */
@property (nonatomic, readonly) CGFloat scale;

/**
 *  The actual frame of the displayed image.
 */
@property (nonatomic, readonly) CGRect pictureFrame;

/**
 *  Wheters the image will be scaled to fill the view
 *  or not.
 */
@property (nonatomic, readwrite) BOOL fitToView;

/**
 *  If enabled, the view will show a white border around the image.
 */
@property (nonatomic, readwrite) BOOL showBorder;

/**
 * Given the size of the preview image to be shown, returns the best possible
 * size for the view.
 */
- (NSSize)optimalViewSizeForImageSize:(NSSize)imageSize minSize:(NSSize)minSize;

@end

NS_ASSUME_NONNULL_END
