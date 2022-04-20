/*  HBPicture+UIAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPicture+UIAdditions.h"
#import "HBTitle.h"
#import "HBLocalizationUtilities.h"

#include "handbrake/handbrake.h"

@implementation HBPicture (UIAdditions)

#pragma mark - Editable state

+ (NSSet<NSString *> *)keyPathsForValuesAffectingCustomCropEnabled
{
    return [NSSet setWithObjects:@"cropMode", nil];
}

- (BOOL)isCustomCropEnabled
{
    return self.cropMode == HBPictureCropModeCustom;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingCustomResolutionLimitEnabled
{
    return [NSSet setWithObjects:@"resolutionLimitMode", nil];
}

- (BOOL)isCustomResolutionLimitEnabled
{
    return self.resolutionLimitMode == HBPictureResolutionLimitModeCustom;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingCustomAnamorphicEnabled
{
    return [NSSet setWithObjects:@"anamorphicMode", nil];
}

- (BOOL)isCustomAnamorphicEnabled
{
    return self.anamorphicMode == HBPictureAnarmophicModeCustom;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingCustomPadEnabled
{
    return [NSSet setWithObjects:@"padMode", nil];
}

- (BOOL)isCustomPadEnabled
{
    return self.padMode == HBPicturePadModeCustom;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingCustomPadColorEnabled
{
    return [NSSet setWithObjects:@"padColorMode", nil];
}

- (BOOL)isCustomPadColorEnabled
{
    return self.padColorMode == HBPicturePadColorModeCustom;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingInfo
{
    return [NSSet setWithObjects:@"parWidth", @"parHeight", @"displayWidth", @"width", @"height",@"anamorphicMode", @"cropTop", @"cropBottom", @"cropLeft", @"cropRight", nil];
}

#pragma mark - Labels

- (NSString *)sourceStorageSize
{
    return [NSString stringWithFormat:@"%dx%d", self.sourceWidth, self.sourceHeight];
}

- (NSString *)sourceDisplaySize
{
    return [NSString stringWithFormat:@"%dx%d", self.sourceDisplayWidth, self.sourceHeight];
}

- (NSString *)sourceAspectRatio
{
    return FormattedDisplayAspect(self.sourceWidth * self.sourceParNum, self.sourceHeight * self.sourceParDen);
}

static NSString * FormattedDisplayAspect(double disp_width, double disp_height)
{
    NSString *str;

    int iaspect = disp_width * 9 / disp_height;
    if (disp_width / disp_height > 1.9)
    {
        // x.x:1
        str = [NSString stringWithFormat:@"%.2f:1", disp_width / disp_height];
    }
    else if (iaspect >= 15)
    {
        // x.x:9
        str = [NSString stringWithFormat:@"%.4g:9", disp_width * 9 / disp_height];
    }
    else if (iaspect >= 9)
    {
        // x.x:3
        str = [NSString stringWithFormat:@"%.4g:3", disp_width * 3 / disp_height];
    }
    else
    {
        // 1:x.x
        str = [NSString stringWithFormat:@"1:%.2f", disp_height / disp_width];
    }
    return str;
}

- (NSString *)storageSize
{
    return [NSString stringWithFormat:@"%dx%d", self.storageWidth, self.storageHeight];
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingStorageSize
{
    return [NSSet setWithObjects:@"storageWidth", @"storageHeight", nil];
}

- (NSString *)displaySize
{
    return [NSString stringWithFormat:@"%dx%d", self.displayWidth, self.displayHeight];
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingDisplaySize
{
    return [NSSet setWithObjects:@"displayWidth", @"displayHeight", nil];
}

- (NSString *)displayAspectRatio
{
    return FormattedDisplayAspect(self.displayWidth, self.displayHeight);
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingDisplayAspectRatio
{
    return [NSSet setWithObjects:@"displayWidth", @"displayHeight", nil];
}

- (NSString *)info
{
    NSString *sizeInfo = @"";

    sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                (@"Source: %dx%d, ", @"HBPicture -> short info"),
                self.sourceWidth, self.sourceHeight];

    if (self.anamorphicMode == HBPictureAnarmophicModeAuto && self.parNum != 1 && self.parDen != 1)
    {
        sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                    (@"%@Output: %dx%d, Anamorphic: %dx%d Automatic", @"HBPicture -> short info"),
                    sizeInfo, self.storageWidth, self.storageHeight, self.displayWidth, self.displayHeight];
    }
    else if (self.anamorphicMode == HBPictureAnarmophicModeCustom && self.parNum != 1 && self.parDen != 1)
    {
        sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                    (@"%@Output: %dx%d, Anamorphic: %dx%d Custom", @"HBPicture -> short info"),
                    sizeInfo, self.storageWidth, self.storageHeight, self.displayWidth, self.displayHeight];
    }
    else // No Anamorphic
    {
        sizeInfo = [NSString stringWithFormat:HBKitLocalizedString
                    (@"%@Output: %dx%d", @"HBPicture -> short info"),
                    sizeInfo, self.storageWidth, self.storageHeight];
    }

    return sizeInfo;
}

- (NSString *)shortInfo
{
    if (self.parNum != 1 && self.parDen != 1)
    {
        return [NSString stringWithFormat:HBKitLocalizedString(@"%dx%d Storage, %dx%d Display", @"HBPicture -> short info"),
                self.storageWidth, self.storageHeight, self.displayWidth, self.height];
    }
    else
    {
        return [NSString stringWithFormat:HBKitLocalizedString(@"%dx%d", @"HBPicture -> short non anamorphic info"), self.storageWidth, self.storageHeight];
    }
}

- (NSString *)summary
{
    NSMutableString *summary = [NSMutableString stringWithString:@""];
    [summary appendString:self.info];
    if (self.cropMode != HBPictureCropModeNone && (self.cropTop && self.cropBottom && self.cropLeft && self.cropRight))
    {
        [summary appendFormat:HBKitLocalizedString(@", Crop: %@ %d/%d/%d/%d", @"HBPicture -> summary"),
         self.cropMode ? HBKitLocalizedString(@"Automatic", @"HBPicture -> summary") : HBKitLocalizedString(@"Custom", @"HBPicture -> summary"),
         self.cropTop, self.cropBottom, self.cropLeft, self.cropRight];
    }
    if (self.padMode != HBPicturePadModeNone && (self.padTop && self.padBottom && self.padLeft && self.padRight))
    {
        [summary appendFormat:HBKitLocalizedString(@", Border: %d/%d/%d/%d", @"HBPicture -> summary"),
         self.padTop, self.padBottom, self.padLeft, self.padRight];
    }

    return [summary copy];
}

@end
