/*  HBImageUtilities.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

CF_ASSUME_NONNULL_BEGIN

CGImageRef __nullable CreateScaledCGImageFromCGImage(CGImageRef image, CGFloat thumbnailHeight);
CGImageRef __nullable CGImageRotated(CGImageRef imgRef, CGFloat angle, BOOL flipped) CF_RETURNS_RETAINED;
CGColorSpaceRef copyColorSpace(int primaries, int transfer, int matrix);

CF_ASSUME_NONNULL_END

