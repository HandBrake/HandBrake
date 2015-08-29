/*  HBPicture+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPicture+UIAdditions.h"
#import "HBTitle.h"
#include "hb.h"

@implementation HBPicture (UIAdditions)

@dynamic maxHeight;
@dynamic maxWidth;

@dynamic maxHorizontalCrop;
@dynamic maxVerticalCrop;

@dynamic keepDisplayAspectEditable;

#pragma mark - Editable state

- (BOOL)isWidthEditable
{
    return (self.anamorphicMode != HB_ANAMORPHIC_STRICT) ? YES : NO;
}

- (BOOL)isHeightEditable
{
    return (self.anamorphicMode != HB_ANAMORPHIC_STRICT) ? YES : NO;
}

- (BOOL)isKeepDisplayAspectEditable
{
    if (self.anamorphicMode == HB_ANAMORPHIC_STRICT ||
        self.anamorphicMode == HB_ANAMORPHIC_LOOSE)
    {
        return NO;
    }
    else
    {
        return YES;
    }
}

- (BOOL)isCustomAnamorphicEnabled
{
    return self.anamorphicMode == HB_ANAMORPHIC_CUSTOM;
}

- (NSString *)info
{
    NSString *sizeInfo = @"";

    sizeInfo = [NSString stringWithFormat:
                @"Source: %dx%d, ",
                self.sourceWidth, self.sourceHeight];

    if (self.anamorphicMode == HB_ANAMORPHIC_STRICT) // Original PAR Implementation
    {
        sizeInfo = [NSString stringWithFormat:
                    @"%@Output: %dx%d, Anamorphic: %dx%d Strict",
                    sizeInfo, self.width, self.height, self.displayWidth, self.height];
    }
    else if (self.anamorphicMode == HB_ANAMORPHIC_LOOSE) // Loose Anamorphic
    {
        sizeInfo = [NSString stringWithFormat:
                    @"%@Output: %dx%d, Anamorphic: %dx%d Loose",
                    sizeInfo, self.width, self.height, self.displayWidth, self.height];
    }
    else if (self.anamorphicMode == HB_ANAMORPHIC_CUSTOM) // Custom Anamorphic
    {
        sizeInfo = [NSString stringWithFormat:
                    @"%@Output: %dx%d, Anamorphic: %dx%d Custom",
                    sizeInfo, self.width, self.height, self.displayWidth, self.height];
    }
    else // No Anamorphic
    {
        sizeInfo = [NSString stringWithFormat:
                    @"%@Output: %dx%d",
                    sizeInfo, self.width, self.height];
    }

    return sizeInfo;
}

- (NSString *)sourceInfo
{
    NSString *sizeInfo = @"";

    sizeInfo = [NSString stringWithFormat:@"%d x %d",  self.sourceWidth, self.sourceHeight];

    if (self.sourceWidth != self.sourceDisplayWidth)
    {
        sizeInfo = [NSString stringWithFormat:@"%d x %d, Anamorphic: %d x %d", self.sourceWidth, self.sourceHeight, self.sourceDisplayWidth, self.sourceHeight];
    }

    return sizeInfo;
}

- (NSString *)summary
{
    NSMutableString *summary = [NSMutableString stringWithString:@""];
    [summary appendString:self.info];

    if (self.anamorphicMode != HB_ANAMORPHIC_STRICT)
    {
        // anamorphic is not Strict, show the modulus
        [summary appendFormat:@", Modulus: %d", self.modulus];
    }

    [summary appendFormat:@", Crop: %s %d/%d/%d/%d",
     self.autocrop ? "Auto" : "Custom",
     self.cropTop, self.cropBottom,
     self.cropLeft, self.cropRight];

    return [summary copy];
}

@end
