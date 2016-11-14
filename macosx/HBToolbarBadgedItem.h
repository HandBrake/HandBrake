/* HBToolbarBadgedItem.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License.
 */

#import <Cocoa/Cocoa.h>

@interface HBToolbarBadgedItem : NSToolbarItem

@property (nonatomic, copy) NSString *badgeValue;

@property (nonatomic, copy) NSColor *badgeTextColor;
@property (nonatomic, copy) NSColor *badgeFillColor;

@end
