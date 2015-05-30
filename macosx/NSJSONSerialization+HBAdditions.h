/*  NSJSONSerialization+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSJSONSerialization (HBAdditions)

+ (nullable id)HB_JSONObjectWithUTF8String:(const char *)nullTerminatedCString options:(NSJSONReadingOptions)opt error:(NSError **)error;
+ (nullable NSString *)HB_StringWithJSONObject:(id)obj options:(NSJSONWritingOptions)opt error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END
