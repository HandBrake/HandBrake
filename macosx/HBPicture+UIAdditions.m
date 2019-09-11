/*  HBPicture+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPicture+UIAdditions.h"
#import "HBTitle.h"
#import "HBLocalizationUtilities.h"

#include "handbrake/handbrake.h"

@implementation HBPicture (UIAdditions)

@dynamic maxHeight;
@dynamic maxWidth;

@dynamic maxTopCrop;
@dynamic maxBottomCrop;
@dynamic maxLeftCrop;
@dynamic maxRightCrop;

#pragma mark - Editable state

+ (NSSet<NSString *> *)keyPathsForValuesAffectingKeepDisplayAspectEditable
{
    return [NSSet setWithObjects:@"anamorphicMode", nil];
}

- (BOOL)isKeepDisplayAspectEditable
{
    if (self.anamorphicMode == HB_ANAMORPHIC_AUTO ||
        self.anamorphicMode == HB_ANAMORPHIC_LOOSE)
    {
        return NO;
    }
    else
    {
        return YES;
    }
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingCustomAnamorphicEnabled
{
    return [NSSet setWithObjects:@"anamorphicMode", nil];
}

- (BOOL)isCustomAnamorphicEnabled
{
    return self.anamorphicMode == HB_ANAMORPHIC_CUSTOM;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingInfo
{
    return [NSSet setWithObjects:@"parWidth", @"parHeight", @"displayWidth", @"width", @"height",@"anamorphicMode", @"cropTop", @"cropBottom", @"cropLeft", @"cropRight", nil];
}

- (NSString *)info
{
    NSString *sizeInfo = @"";

    sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                (@"Source: %dx%d, ", @"HBPicture -> short info"),
                self.sourceWidth, self.sourceHeight];

    if (self.anamorphicMode == HB_ANAMORPHIC_AUTO)
    {
        sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                    (@"%@Output: %dx%d, Anamorphic: %dx%d Auto", @"HBPicture -> short info"),
                    sizeInfo, self.width, self.height, self.displayWidth, self.height];
    }
    else if (self.anamorphicMode == HB_ANAMORPHIC_LOOSE) // Loose Anamorphic
    {
        sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                    (@"%@Output: %dx%d, Anamorphic: %dx%d Loose", @"HBPicture -> short info"),
                    sizeInfo, self.width, self.height, self.displayWidth, self.height];
    }
    else if (self.anamorphicMode == HB_ANAMORPHIC_CUSTOM) // Custom Anamorphic
    {
        sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                    (@"%@Output: %dx%d, Anamorphic: %dx%d Custom", @"HBPicture -> short info"),
                    sizeInfo, self.width, self.height, self.displayWidth, self.height];
    }
    else // No Anamorphic
    {
        sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                    (@"%@Output: %dx%d", @"HBPicture -> short info"),
                    sizeInfo, self.width, self.height];
    }

    return sizeInfo;
}

- (NSString *)shortInfo
{
    return [NSString stringWithFormat:HBKitLocalizedString(@"%dx%d Storage, %dx%d Display", @"HBPicture -> short info"), self.width, self.height, self.displayWidth, self.height];
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingSummary
{
    return [NSSet setWithObjects:@"parWidth", @"parHeight", @"displayWidth", @"width", @"height",@"anamorphicMode", @"cropTop", @"cropBottom", @"cropLeft", @"cropRight", nil];
}

- (NSString *)summary
{
    NSMutableString *summary = [NSMutableString stringWithString:@""];
    [summary appendString:self.info];
    [summary appendFormat:HBKitLocalizedString(@", Modulus: %d", @"HBPicture -> summary"), self.modulus];
    [summary appendFormat:HBKitLocalizedString(@", Crop: %@ %d/%d/%d/%d", @"HBPicture -> summary"),
     self.autocrop ? HBKitLocalizedString(@"Auto", @"HBPicture -> summary") : HBKitLocalizedString(@"Custom", @"HBPicture -> summary"),
     self.cropTop, self.cropBottom,
     self.cropLeft, self.cropRight];

    return [summary copy];
}

@end
