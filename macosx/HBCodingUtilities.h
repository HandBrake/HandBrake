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
#define decodeObject(x, cl) x = [decoder decodeObjectOfClass:[cl class] forKey:OBJC_STRINGIFY(x)];

#define fail(x) [decoder failWithError:[NSError errorWithDomain:@"HBKitErrorDomain" code:1 userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Failed to decode %@", OBJC_STRINGIFY(x)]}]]; goto fail;

#define decodeCollectionOfObjects(x, cl, objectcl) x = [decoder decodeObjectOfClasses:[NSSet setWithObjects:[cl class], [objectcl class], nil] forKey:OBJC_STRINGIFY(x)];

#define decodeCollectionOfObjectsOrFail(x, cl, objectcl) x = [decoder decodeObjectOfClasses:[NSSet setWithObjects:[cl class], [objectcl class], nil] forKey:OBJC_STRINGIFY(x)]; if (x == nil || ![x isKindOfClass:[cl class]]) { fail(x) }

#define decodeObjectOrFail(x, cl) x = [decoder decodeObjectOfClass:[cl class] forKey:OBJC_STRINGIFY(x)]; if (x == nil) { fail(x) }
