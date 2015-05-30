/*  NSJSONSerialization+HBAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "NSJSONSerialization+HBAdditions.h"

@implementation NSJSONSerialization (HBAdditions)

+ (id)HB_JSONObjectWithUTF8String:(const char *)nullTerminatedCString options:(NSJSONReadingOptions)opt error:(NSError **)error;
{
    NSData *data = [NSData dataWithBytes:nullTerminatedCString length:strlen(nullTerminatedCString)];
    id result = [NSJSONSerialization JSONObjectWithData:data options:opt error:error];
    return result;
}

+ (NSString *)HB_StringWithJSONObject:(id)obj options:(NSJSONWritingOptions)opt error:(NSError **)error
{
    NSData *data = [NSJSONSerialization dataWithJSONObject:obj options:opt error:error];
    if (data)
    {
        return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    }

    return nil;
}

@end
