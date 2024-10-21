/* HBPreferencesWindow.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License.
 */

#import "HBPreferencesWindow.h"

@implementation HBPreferencesWindow

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
    if (menuItem.action == @selector(toggleToolbarShown:))
    {
        return NO;
    }
    return [super validateMenuItem:menuItem];
}

@end
