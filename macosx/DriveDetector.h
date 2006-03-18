/* DriveDetector.h $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@interface DriveDetector : NSObject
{
    id             fTarget;
    SEL            fSelector;
    
    int            fCount;
    NSMutableArray * fDrives;
    NSTimer        * fTimer;
}

- (id) initWithCallback: (id) target selector: (SEL) selector;

@end
