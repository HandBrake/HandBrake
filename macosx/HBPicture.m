/*  HBPicture.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPicture.h"
#import "HBTitle.h"

#import "HBCodingUtilities.h"

#include "hb.h"

NSString * const HBPictureChangedNotification = @"HBPictureChangedNotification";

@interface HBPicture ()

@property (nonatomic, readwrite, getter=isValidating) BOOL validating;
@property (nonatomic, readwrite, getter=areNotificationsEnabled) BOOL notificationsEnabled;

@property (nonatomic, readwrite) int keep;
@property (nonatomic, readwrite) BOOL darUpdated;

@property (nonatomic, readonly) int sourceParNum;
@property (nonatomic, readonly) int sourceParDen;

@property (nonatomic, readonly) int autoCropTop;
@property (nonatomic, readonly) int autoCropBottom;
@property (nonatomic, readonly) int autoCropLeft;
@property (nonatomic, readonly) int autoCropRight;

@end

@implementation HBPicture

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        // Set some values if we ever need a fake instance
        _width = 1280;
        _height = 720;

        _sourceWidth = 1280;
        _sourceHeight = 720;

        _anamorphicMode = HB_ANAMORPHIC_NONE;

        _parWidth = 1;
        _parHeight = 1;
        _sourceParNum = 1;
        _sourceParDen = 1;

    }
    return self;
}

- (instancetype)initWithTitle:(HBTitle *)title
{
    self = [self init];
    if (self)
    {
        _width = title.width;
        _height = title.height;

        _sourceWidth = title.width;
        _sourceHeight = title.height;

        _sourceParNum = title.parWidth;
        _sourceParDen = title.parHeight;

        _autoCropTop    = title.autoCropTop;
        _autoCropBottom = title.autoCropBottom;
        _autoCropLeft   = title.autoCropLeft;
        _autoCropRight  = title.autoCropRight;

        [self validateSettings];

        _notificationsEnabled = YES;
    }
    return self;
}

- (void)postChangedNotification
{
    if (self.areNotificationsEnabled)
    {
        [[NSNotificationCenter defaultCenter] postNotification: [NSNotification notificationWithName:HBPictureChangedNotification
                                                                                              object:self
                                                                                            userInfo:nil]];
    }
}

- (void)setWidth:(int)width
{
    if (width != _width)
    {
        [[self.undo prepareWithInvocationTarget:self] setWidth:_width];
    }
    _width = width;

    if (!self.isValidating)
    {
        self.keep |= HB_KEEP_WIDTH;
        [self validateSettings];
    }
}

- (BOOL)validateWidth:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];
        int roundedValue = value - (value % self.modulus);

        if (value >= self.maxWidth)
        {
            *ioValue = @(self.maxWidth);
        }
        else if (value <= 32)
        {
            *ioValue = @32;
        }
        else if (value != roundedValue)
        {
            *ioValue = @(roundedValue);
        }
    }

    return retval;
}

- (void)setHeight:(int)height
{
    if (height != _height)
    {
        [[self.undo prepareWithInvocationTarget:self] setHeight:_height];
    }
    _height = height;
    if (!self.isValidating)
    {
        self.keep |= HB_KEEP_HEIGHT;
        [self validateSettings];
    }
}

- (BOOL)validateHeight:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];
        int roundedValue = value - (value % self.modulus);

        if (value >= self.maxHeight)
        {
            *ioValue = @(self.maxHeight);
        }
        else if (value <= 32)
        {
            *ioValue = @32;
        }
        else if (value != roundedValue)
        {
            *ioValue = @(roundedValue);
        }
    }

    return retval;
}

- (void)setDisplayWidth:(int)displayWidth
{
    if (displayWidth != _displayWidth)
    {
        [[self.undo prepareWithInvocationTarget:self] setDisplayWidth:_displayWidth];
    }
    _displayWidth = displayWidth;
    if (!self.isValidating)
    {
        self.darUpdated = YES;
        [self validateSettings];
    }
}

- (void)setParWidth:(int)parWidth
{
    if (parWidth != _parWidth)
    {
        [[self.undo prepareWithInvocationTarget:self] setParWidth:_parWidth];
    }
    _parWidth = parWidth;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setParHeight:(int)parHeight
{
    if (parHeight != _parHeight)
    {
        [[self.undo prepareWithInvocationTarget:self] setParHeight:_parHeight];
    }
    _parHeight = parHeight;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setCropTop:(int)cropTop
{
    if (cropTop != _cropTop)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropTop:_cropTop];
    }
    _cropTop = cropTop;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setCropBottom:(int)cropBottom
{
    if (cropBottom != _cropBottom)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropBottom:_cropBottom];
    }
    _cropBottom = cropBottom;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setCropLeft:(int)cropLeft
{
    if (cropLeft != _cropLeft)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropLeft:_cropLeft];
    }
    _cropLeft = cropLeft;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setCropRight:(int)cropRight
{
    if (cropRight != _cropRight)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropRight:_cropRight];
    }
    _cropRight = cropRight;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (BOOL)validateCropTop:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validateVCrop:ioValue];
    return YES;
}

- (BOOL)validateCropBottom:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validateVCrop:ioValue];
    return YES;
}

- (BOOL)validateCropLeft:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validateHCrop:ioValue];
    return YES;
}

- (BOOL)validateCropRight:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validateHCrop:ioValue];
    return YES;
}

- (void)validateHCrop:(NSNumber **)ioValue
{
    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];
        if (value >= self.maxHorizontalCrop)
        {
            *ioValue =  @(self.maxHorizontalCrop);
        }
        else if (value < 0)
        {
            *ioValue = @0;
        }
    }
}

- (void)validateVCrop:(NSNumber **)ioValue
{
    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];
        if (value >= self.maxVerticalCrop)
        {
            *ioValue =  @(self.maxVerticalCrop);
        }
        else if (value < 0)
        {
            *ioValue = @0;
        }
    }
}

- (void)setAutocrop:(BOOL)autocrop
{
    if (autocrop != _autocrop)
    {
        [[self.undo prepareWithInvocationTarget:self] setAutocrop:_autocrop];
    }
    _autocrop = autocrop;
    if (autocrop && !self.isValidating)
    {
        if (!(self.undo.isUndoing || self.undo.isRedoing))
        {
            self.validating = YES;
            // Reset the crop values to those determined right after scan
            self.cropTop    = self.autoCropTop;
            self.cropBottom = self.autoCropBottom;
            self.cropLeft   = self.autoCropLeft;
            self.cropRight  = self.autoCropRight;
            self.validating = NO;
        }
        [self validateSettings];
    }
}

- (void)setAnamorphicMode:(int)anamorphicMode
{
    if (anamorphicMode != _anamorphicMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setAnamorphicMode:_anamorphicMode];
    }
    _anamorphicMode = anamorphicMode;

    if (self.anamorphicMode == HB_ANAMORPHIC_STRICT ||
        self.anamorphicMode == HB_ANAMORPHIC_LOOSE)
    {
        self.keepDisplayAspect = YES;
    }

    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setKeepDisplayAspect:(BOOL)keepDisplayAspect
{
    if (keepDisplayAspect != _keepDisplayAspect)
    {
        [[self.undo prepareWithInvocationTarget:self] setKeepDisplayAspect:_keepDisplayAspect];
    }
    _keepDisplayAspect = keepDisplayAspect;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setModulus:(int)modulus
{
    if (modulus != _modulus)
    {
        [[self.undo prepareWithInvocationTarget:self] setModulus:_modulus];
    }
    _modulus = modulus;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

#pragma mark - Max sizes

- (int)maxWidth
{
    return self.sourceWidth - self.cropRight - self.cropLeft;
}

- (int)maxHeight
{
    return self.sourceHeight - self.cropTop - self.cropBottom;
}

- (int)maxVerticalCrop
{
    return self.sourceHeight / 2 - 2;
}

- (int)maxHorizontalCrop
{
    return self.sourceWidth / 2 - 2;
}

- (int)sourceDisplayWidth
{
    return (int) (self.sourceWidth * self.sourceParNum / (double)self.sourceParDen);
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    // Tell KVO to reload the editable state.
    if ([key isEqualToString:@"keepDisplayAspectEditable"] ||
        [key isEqualToString:@"heightEditable"] ||
        [key isEqualToString:@"widthEditable"] ||
        [key isEqualToString:@"customAnamorphicEnabled"])
    {
        retval = [NSSet setWithObjects:@"anamorphicMode", nil];
    }

    if ([key isEqualToString:@"maxWidth"] ||
        [key isEqualToString:@"maxHeight"])
    {
        retval = [NSSet setWithObjects:@"cropTop", @"cropBottom", @"cropLeft", @"cropRight", nil];
    }

    if ([key isEqualToString:@"info"] || [key isEqualToString:@"summary"])
    {
        retval = [NSSet setWithObjects:@"parWidth", @"parHeight", @"displayWidth", @"width", @"height",@"anamorphicMode", @"cropTop", @"cropBottom", @"cropLeft", @"cropRight", nil];
    }

    return retval;
}

- (void)setNilValueForKey:(NSString *)key
{
    if ([key isEqualToString:@"width"] || [key isEqualToString:@"height"])
    {
        [self setValue:@64 forKey:key];
    }
    else
    {
        [self setValue:@0 forKey:key];
    }
}

#pragma mark - Picture Update Logic

/**
 *  Validates the settings through hb_set_anamorphic_size2,
 *  each setters calls this after setting its value.
 */
- (void)validateSettings
{
    self.validating = YES;
    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        self.keep |= self.keepDisplayAspect * HB_KEEP_DISPLAY_ASPECT;

        hb_geometry_t srcGeo, resultGeo;
        hb_geometry_settings_t uiGeo;

        srcGeo.width = self.sourceWidth;
        srcGeo.height = self.sourceHeight;
        srcGeo.par.num = self.sourceParNum;
        srcGeo.par.den = self.sourceParDen;

        uiGeo.mode = self.anamorphicMode;
        uiGeo.keep = self.keep;
        uiGeo.itu_par = 0;
        uiGeo.modulus = self.modulus;

        int crop[4] = {self.cropTop, self.cropBottom, self.cropLeft, self.cropRight};
        memcpy(uiGeo.crop, crop, sizeof(int[4]));
        uiGeo.geometry.width = self.width;
        uiGeo.geometry.height =  self.height;
        // Modulus added to maxWidth/maxHeight to allow a small amount of
        // upscaling to the next mod boundary.
        uiGeo.maxWidth = self.sourceWidth - crop[2] - crop[3] + self.modulus - 1;
        uiGeo.maxHeight = self.sourceHeight - crop[0] - crop[1] + self.modulus - 1;

        hb_rational_t par = {self.parWidth, self.parHeight};
        uiGeo.geometry.par = par;
        if (self.anamorphicMode == HB_ANAMORPHIC_CUSTOM && self.darUpdated)
        {
            uiGeo.geometry.par.num = self.displayWidth;
            uiGeo.geometry.par.den = uiGeo.geometry.width;
        }
        hb_set_anamorphic_size2(&srcGeo, &uiGeo, &resultGeo);

        int display_width;
        display_width = resultGeo.width * resultGeo.par.num / resultGeo.par.den;

        self.width = resultGeo.width;
        self.height = resultGeo.height;
        self.parWidth = resultGeo.par.num;
        self.parHeight = resultGeo.par.den;
        self.displayWidth = display_width;
    }
    self.validating = NO;
    self.keep = 0;
    self.darUpdated = NO;

    [self postChangedNotification];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBPicture *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_width = _width;
        copy->_height = _height;

        copy->_keepDisplayAspect = _keepDisplayAspect;
        copy->_anamorphicMode = _anamorphicMode;
        copy->_modulus = _modulus;

        copy->_displayWidth = _displayWidth;
        copy->_parWidth = _parWidth;
        copy->_parHeight = _parHeight;

        copy->_autocrop = _autocrop;
        copy->_cropTop = _cropTop;
        copy->_cropBottom = _cropBottom;
        copy->_cropLeft = _cropLeft;
        copy->_cropRight = _cropRight;

        copy->_autoCropTop = _autoCropTop;
        copy->_autoCropBottom = _autoCropBottom;
        copy->_autoCropLeft = _autoCropLeft;
        copy->_autoCropRight = _autoCropRight;

        copy->_sourceWidth = _sourceWidth;
        copy->_sourceHeight = _sourceHeight;
        copy->_sourceParNum = _sourceParNum;
        copy->_sourceParDen = _sourceParDen;

        copy->_notificationsEnabled = _notificationsEnabled;
    }

    return copy;
}

#pragma mark - NSCoding

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBPictureVersion"];

    encodeInt(_width);
    encodeInt(_height);

    encodeBool(_keepDisplayAspect);
    encodeInt(_anamorphicMode);
    encodeInt(_modulus);

    encodeInt(_displayWidth);
    encodeInt(_parWidth);
    encodeInt(_parHeight);

    encodeBool(_autocrop);
    encodeInt(_cropTop);
    encodeInt(_cropBottom);
    encodeInt(_cropLeft);
    encodeInt(_cropRight);

    encodeInt(_autoCropTop);
    encodeInt(_autoCropBottom);
    encodeInt(_autoCropLeft);
    encodeInt(_autoCropRight);

    encodeInt(_sourceWidth);
    encodeInt(_sourceHeight);
    encodeInt(_sourceParNum);
    encodeInt(_sourceParDen);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_width);
    decodeInt(_height);

    decodeBool(_keepDisplayAspect);
    decodeInt(_anamorphicMode);
    decodeInt(_modulus);

    decodeInt(_displayWidth);
    decodeInt(_parWidth);
    decodeInt(_parHeight);

    decodeBool(_autocrop);
    decodeInt(_cropTop);
    decodeInt(_cropBottom);
    decodeInt(_cropLeft);
    decodeInt(_cropRight);

    decodeInt(_autoCropTop);
    decodeInt(_autoCropBottom);
    decodeInt(_autoCropLeft);
    decodeInt(_autoCropRight);

    decodeInt(_sourceWidth);
    decodeInt(_sourceHeight);
    decodeInt(_sourceParNum);
    decodeInt(_sourceParDen);

    _notificationsEnabled = YES;
    
    return self;
}

#pragma mark - Presets

- (void)writeToPreset:(HBMutablePreset *)preset
{
    preset[@"PictureKeepRatio"] = @(self.keepDisplayAspect);
    preset[@"PictureModulus"]   = @(self.modulus);

    switch (self.anamorphicMode) {
        case HB_ANAMORPHIC_NONE:
            preset[@"PicturePAR"] = @"off";
            break;
        case HB_ANAMORPHIC_LOOSE:
            preset[@"PicturePAR"] = @"loose";
            break;
        case HB_ANAMORPHIC_STRICT:
            preset[@"PicturePAR"] = @"strict";
            break;
        case HB_ANAMORPHIC_CUSTOM:
            preset[@"PicturePAR"] = @"custom";
            break;
        default:
            preset[@"PicturePAR"] = @"loose";
            break;
    }

    // PAR
    preset[@"PicturePARWidth"] = @(self.parWidth);
    preset[@"PicturePARHeight"] = @(self.parHeight);

    // Set crop settings
    preset[@"PictureAutoCrop"] = @(self.autocrop);

    preset[@"PictureTopCrop"]    = @(self.cropTop);
    preset[@"PictureBottomCrop"] = @(self.cropBottom);
    preset[@"PictureLeftCrop"]   = @(self.cropLeft);
    preset[@"PictureRightCrop"]  = @(self.cropRight);
}

- (void)applyPreset:(HBPreset *)preset
{
    self.validating = YES;
    self.notificationsEnabled = NO;

    /* Note: objectForKey:@"UsesPictureSettings" refers to picture size, which encompasses:
     * height, width, keep ar, anamorphic and crop settings.
     * picture filters are handled separately below.
     */
    int maxWidth = self.sourceWidth - self.cropLeft - self.cropRight;
    int maxHeight = self.sourceHeight - self.cropTop - self.cropBottom;
    int parWidth = self.parWidth;
    int parHeight = self.parHeight;
    int jobMaxWidth = 0, jobMaxHeight = 0;

    /* Check to see if the objectForKey:@"UsesPictureSettings is greater than 0, as 0 means use picture sizing "None"
     * ( 2 is use max for source and 1 is use exact size when the preset was created ) and the
     * preset completely ignores any picture sizing values in the preset.
     */
    if ([preset[@"UsesPictureSettings"] intValue] > 0)
    {
        // If Cropping is set to custom, then recall all four crop values from
        // when the preset was created and apply them
        if ([preset[@"PictureAutoCrop"] intValue] == 0)
        {
            self.autocrop = NO;

            // Here we use the custom crop values saved at the time the preset was saved
            self.cropTop    = [preset[@"PictureTopCrop"] intValue];
            self.cropBottom = [preset[@"PictureBottomCrop"] intValue];
            self.cropLeft   = [preset[@"PictureLeftCrop"] intValue];
            self.cropRight  = [preset[@"PictureRightCrop"] intValue];
        }
        else // if auto crop has been saved in preset, set to auto and use post scan auto crop
        {
            self.autocrop = YES;
            /* Here we use the auto crop values determined right after scan */
            self.cropTop    = self.autoCropTop;
            self.cropBottom = self.autoCropBottom;
            self.cropLeft   = self.autoCropLeft;
            self.cropRight  = self.autoCropRight;
        }

        // crop may have changed, reset maxWidth/maxHeight
        maxWidth = self.sourceWidth - self.cropLeft - self.cropRight;
        maxHeight = self.sourceHeight - self.cropTop - self.cropBottom;

        // Set modulus
        if (preset[@"PictureModulus"])
        {
            self.modulus = [preset[@"PictureModulus"]  intValue];
        }
        else
        {
            self.modulus = 16;
        }

        // Assume max picture settings initially.
        self.keepDisplayAspect = [preset[@"PictureKeepRatio"] boolValue];

        if ([preset[@"PicturePAR"] isEqualToString:@"off"])
        {
            self.anamorphicMode = HB_ANAMORPHIC_NONE;
        }
        else if ([preset[@"PicturePAR"] isEqualToString:@"strict"])
        {
            self.anamorphicMode = HB_ANAMORPHIC_STRICT;
        }
        else if ([preset[@"PicturePAR"] isEqualToString:@"custom"])
        {
            self.anamorphicMode = HB_ANAMORPHIC_CUSTOM;
            parWidth = [preset[@"PicturePARWidth"] intValue];
            parHeight = [preset[@"PicturePARHeight"] intValue];
        }
        else
        {
            self.anamorphicMode = HB_ANAMORPHIC_LOOSE;
        }

        self.width = self.sourceWidth - self.cropLeft - self.cropRight;
        self.height = self.sourceHeight - self.cropTop - self.cropBottom;

        // Check to see if the "UsesPictureSettings" is 2,
        // which means "Use max. picture size for source"
        // If not 2 it must be 1 here which means "Use the picture
        // size specified in the preset"
        if ([preset[@"UsesPictureSettings"] intValue] != 2)
        {
             // if the preset specifies neither max. width nor height
             // (both are 0), use the max. picture size
             //
             // if the specified non-zero dimensions exceed those of the
             // source, also use the max. picture size (no upscaling)
            if ([preset[@"PictureWidth"] intValue] > 0)
            {
                jobMaxWidth  = [preset[@"PictureWidth"] intValue];
            }
            if ([preset[@"PictureHeight"] intValue] > 0)
            {
                jobMaxHeight  = [preset[@"PictureHeight"] intValue];
            }
        }
    }
    // Modulus added to maxWidth/maxHeight to allow a small amount of
    // upscaling to the next mod boundary. This does not apply to
    // explicit limits set for device compatibility.  It only applies
    // when limiting to cropped title dimensions.
    maxWidth += self.modulus - 1;
    maxHeight += self.modulus - 1;
    if (jobMaxWidth == 0 || jobMaxWidth > maxWidth)
        jobMaxWidth = maxWidth;
    if (jobMaxHeight == 0 || jobMaxHeight > maxHeight)
        jobMaxHeight = maxHeight;

    hb_geometry_t srcGeo, resultGeo;
    hb_geometry_settings_t uiGeo;

    srcGeo.width = self.sourceWidth;
    srcGeo.height = self.sourceHeight;
    srcGeo.par.num = self.sourceParNum;
    srcGeo.par.den = self.sourceParDen;

    uiGeo.mode = self.anamorphicMode;
    uiGeo.keep = self.keepDisplayAspect * HB_KEEP_DISPLAY_ASPECT;
    uiGeo.itu_par = 0;
    uiGeo.modulus = self.modulus;
    int crop[4] = {self.cropTop, self.cropBottom, self.cropLeft, self.cropRight};
    memcpy(uiGeo.crop, crop, sizeof(int[4]));
    uiGeo.geometry.width = self.width;
    uiGeo.geometry.height =  self.height;

    hb_rational_t par = {parWidth, parHeight};
    uiGeo.geometry.par = par;

    uiGeo.maxWidth = jobMaxWidth;
    uiGeo.maxHeight = jobMaxHeight;
    hb_set_anamorphic_size2(&srcGeo, &uiGeo, &resultGeo);

    int display_width;
    display_width = resultGeo.width * resultGeo.par.num / resultGeo.par.den;

    self.width = resultGeo.width;
    self.height = resultGeo.height;
    self.parWidth = resultGeo.par.num;
    self.parHeight = resultGeo.par.den;
    self.displayWidth = display_width;

    self.validating = NO;
    self.notificationsEnabled = YES;

    [self postChangedNotification];
}

@end
