/*  HBHUD.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */
#import <Foundation/Foundation.h>

@protocol HBHUD <NSObject>

/// Whether the hud can be hidden or not;
- (BOOL)canBeHidden;

// Responder chains is nice and good, but NSViewController
// are removed when the view is hidden, so let's deliver the
// events manually.

- (BOOL)HB_keyDown:(NSEvent *)event;
- (BOOL)HB_scrollWheel:(NSEvent *)theEvent;

@end
