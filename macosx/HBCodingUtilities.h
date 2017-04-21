/*  HBCodingUtilities.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

#define OBJC_STRINGIFY(x) @#x
#define encodeInt(x) [coder encodeInt:x forKey:OBJC_STRINGIFY(x)]
#define encodeInteger(x) [coder encodeInteger:x forKey:OBJC_STRINGIFY(x)]
#define encodeBool(x) [coder encodeBool:x forKey:OBJC_STRINGIFY(x)]
#define encodeDouble(x) [coder encodeDouble:x forKey:OBJC_STRINGIFY(x)]
#define encodeObject(x) [coder encodeObject:x forKey:OBJC_STRINGIFY(x)]

#define decodeInt(x) x = [decoder decodeIntForKey:OBJC_STRINGIFY(x)]
#define decodeInteger(x) x = [decoder decodeIntegerForKey:OBJC_STRINGIFY(x)]
#define decodeBool(x) x = [decoder decodeBoolForKey:OBJC_STRINGIFY(x)]
#define decodeDouble(x) x = [decoder decodeDoubleForKey:OBJC_STRINGIFY(x)]
#define decodeObject(x, cl) x = [HBCodingUtilities decodeObjectOfClass:[cl class] forKey:OBJC_STRINGIFY(x) decoder:decoder];

#define decodeCollectionOfObjects(x, cl, objectcl) x = [HBCodingUtilities decodeObjectOfClasses:[NSSet setWithObjects:[cl class], [objectcl class], nil] forKey:OBJC_STRINGIFY(x) decoder:decoder];

#define decodeCollectionOfObjects2(x, cl, objectcl, objectcl2) x = [HBCodingUtilities decodeObjectOfClasses:[NSSet setWithObjects:[cl class], [objectcl class], [objectcl2 class], nil] forKey:OBJC_STRINGIFY(x) decoder:decoder];

#define decodeCollectionOfObjects3(x, cl, objectcl, objectcl2, objectcl3) x = [HBCodingUtilities decodeObjectOfClasses:[NSSet setWithObjects:[cl class], [objectcl class], [objectcl2 class], [objectcl3 class], nil] forKey:OBJC_STRINGIFY(x) decoder:decoder];


#define decodeObjectOrFail(x, cl) x = [HBCodingUtilities decodeObjectOfClass:[cl class] forKey:OBJC_STRINGIFY(x) decoder:decoder]; if (x == nil) {NSLog(@"Failed to decode: %@", OBJC_STRINGIFY(x)); goto fail;}

NS_ASSUME_NONNULL_BEGIN

@interface HBCodingUtilities : NSObject

/**
 *  Specify what the expected class of the allocated object is. If the coder responds YES to -requiresSecureCoding,
 *  then an exception will be thrown if the class to be decoded does not implement NSSecureCoding or is not isKindOfClass: of the argument.
 *  If the coder responds NO to -requiresSecureCoding, then the class argument is ignored
 *  and no check of the class of the decoded object is performed, exactly as if decodeObjectForKey: had been called.
 *
 *  if NSSecureCoding is not available on the system it check the class after loading the object.
 *
 *  @param aClass  The expect class type.
 *  @param key     The coder key.
 *  @param decoder The NSCoder.
 *
 *  @return the decoder object.
 */
+ (nullable id)decodeObjectOfClass:(Class)aClass forKey:(NSString *)key decoder:(NSCoder *)decoder;


+ (nullable id)decodeObjectOfClasses:(NSSet *)classes forKey:(NSString *)key decoder:(NSCoder *)decoder;

@end

NS_ASSUME_NONNULL_END
