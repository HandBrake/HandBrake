 /**
 * HBDVDDetector.h
 * 8/17/2007
 * 
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.m0k.org/>.
 * It may be used under the terms of the GNU General Public License.
 */

#import <Cocoa/Cocoa.h>


@interface HBDVDDetector : NSObject
{
    NSString *path;
    NSString *bsdName;
}

+ (HBDVDDetector *)detectorForPath: (NSString *)aPath;
- (HBDVDDetector *)initWithPath: (NSString *)aPath;

- (BOOL)isVideoDVD;
- (NSString *)devicePath;

@end