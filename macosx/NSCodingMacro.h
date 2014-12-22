/*  NSCodingMacro.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#ifndef HandBrake_NSCodingMacro_h
#define HandBrake_NSCodingMacro_h

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
#define decodeObject(x) x = [decoder decodeObjectForKey:OBJC_STRINGIFY(x)]

#endif
