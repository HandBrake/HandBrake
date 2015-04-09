 /**
 * HBDVDDetector.h
 * 8/17/2007
 * 
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License.
 */

#import <Cocoa/Cocoa.h>


@interface HBDVDDetector : NSObject
{
    NSString *path;
    NSString *bsdName;
}

+ (HBDVDDetector *)detectorForPath: (NSString *)aPath;
- (HBDVDDetector *)initWithPath: (NSString *)aPath NS_DESIGNATED_INITIALIZER;

@property (nonatomic, getter=isVideoDVD, readonly) BOOL videoDVD;
@property (nonatomic, readonly, copy) NSString *devicePath;

@end