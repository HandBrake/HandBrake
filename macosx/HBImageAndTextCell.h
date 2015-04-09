/* HBImageAndTextCell

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License.
*/

#import <Cocoa/Cocoa.h>

@interface HBImageAndTextCell : NSTextFieldCell

@property (strong) NSImage *image;
@property (nonatomic) NSImageAlignment imageAlignment;
@property (nonatomic) NSSize imageSpacing;

- (void) drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView;
@property (readonly) NSSize cellSize;

@end
