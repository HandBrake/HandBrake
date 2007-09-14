/* HBImageAndTextCell

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.m0k.org/>.
    It may be used under the terms of the GNU General Public License.
*/

#import <Cocoa/Cocoa.h>

@interface HBImageAndTextCell : NSTextFieldCell
{
@private
    NSImage	             *image;
    NSImageAlignment     imageAlignment;    // defaults to NSImageAlignTop. Supports NSImageAlignCenter & NSImageAlignBottom
    NSSize               imageSpacing;      // horizontal and vertical spacing around the image 
}

- (void) setImage:(NSImage *)anImage;
- (NSImage *) image;

- (void) setImageAlignment:(NSImageAlignment)alignment;
- (NSImageAlignment) imageAlignment;

- (void)setImageSpacing:(NSSize)aSize;
- (NSSize)imageSpacing;

- (void) drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView;
- (NSSize) cellSize;

@end
