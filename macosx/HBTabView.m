/* HBTabView

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License.
 */

#import "HBTabView.h"

@implementation HBTabView

-(NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    return [self.dropDelegate draggingEntered:sender];
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    return [self.dropDelegate performDragOperation:sender];
}

- (void)draggingExited:(nullable id <NSDraggingInfo>)sender
{
    [self.dropDelegate draggingExited:sender];
}

@end
