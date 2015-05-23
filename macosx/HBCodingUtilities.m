//
//  HBCodingUtilities.m
//  HandBrake
//
//  Created by Damiano Galassi on 22/04/15.
//
//

#import "HBCodingUtilities.h"

static BOOL useSecureCoding;

@implementation HBCodingUtilities

+ (void)initialize
{
    static BOOL initialized = NO;

    if (!initialized && self == [HBCodingUtilities class])
    {
        useSecureCoding = [NSCoder instancesRespondToSelector:@selector(decodeObjectOfClass:forKey:)] ? YES : NO;
    }
}

+ (id)decodeObjectOfClass:(Class)aClass forKey:(NSString *)key decoder:(NSCoder *)decoder
{
    if (useSecureCoding)
    {
        return [decoder decodeObjectOfClass:aClass forKey:key];
    }
    else
    {
        id obj = [decoder decodeObjectForKey:key];
        if (![obj isKindOfClass:aClass])
        {
            return nil;
        }
        else
        {
            return obj;
        }
    }
}

@end
