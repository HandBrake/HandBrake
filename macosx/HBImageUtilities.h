/*  HBColorUtilities.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

CGImageRef CGImageRotated(CGImageRef imgRef, CGFloat angle, BOOL flipped) CF_RETURNS_RETAINED;
CGColorSpaceRef copyColorSpace(int primaries, int transfer, int matrix);
CGImageRef jnw_decompressedImage(CGImageRef CGImage);
