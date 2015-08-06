 /**
 * HBDVDDetector.h
 * 8/17/2007
 * 
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License.
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBDVDDetector : NSObject

- (instancetype)init NS_UNAVAILABLE;

+ (HBDVDDetector *)detectorForPath: (NSString *)aPath;
- (HBDVDDetector *)initWithPath: (NSString *)aPath NS_DESIGNATED_INITIALIZER;

@property (nonatomic, getter=isVideoDVD, readonly) BOOL videoDVD;
@property (nonatomic, readonly, copy) NSString *devicePath;

@end

NS_ASSUME_NONNULL_END