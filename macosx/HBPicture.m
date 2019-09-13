/*  HBPicture.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPicture.h"
#import "HBTitle.h"

#import "HBCodingUtilities.h"
#import "HBMutablePreset.h"

#include "handbrake/handbrake.h"

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

        _anamorphicMode = HBPictureAnarmophicModeNone;
        _modulus = 2;

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
    [self validateCrop:ioValue max:self.maxTopCrop];
    return YES;
}

- (BOOL)validateCropBottom:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validateCrop:ioValue max:self.maxBottomCrop];
    return YES;
}

- (BOOL)validateCropLeft:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validateCrop:ioValue max:self.maxLeftCrop];
    return YES;
}

- (BOOL)validateCropRight:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validateCrop:ioValue max:self.maxRightCrop];
    return YES;
}

- (void)validateCrop:(NSNumber **)ioValue  max:(int)maxCrop
{
    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];
        if (value >= maxCrop)
        {
            *ioValue =  @(maxCrop);
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

- (void)setAnamorphicMode:(HBPictureAnarmophicMode)anamorphicMode
{
    if (anamorphicMode != _anamorphicMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setAnamorphicMode:_anamorphicMode];
    }
    _anamorphicMode = anamorphicMode;

    if (self.anamorphicMode == HB_ANAMORPHIC_AUTO ||
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

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxWidth
{
    return [NSSet setWithObjects:@"cropTop", @"cropBottom", @"cropLeft", @"cropRight", nil];
}

- (int)maxWidth
{
    return self.sourceWidth - self.cropRight - self.cropLeft;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxHeight
{
    return [NSSet setWithObjects:@"cropTop", @"cropBottom", @"cropLeft", @"cropRight", nil];
}

- (int)maxHeight
{
    return self.sourceHeight - self.cropTop - self.cropBottom;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxTopCrop
{
    return [NSSet setWithObjects:@"cropBottom", nil];
}

- (int)maxTopCrop
{
    return self.sourceHeight - self.cropBottom - 32;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxBottomCrop
{
    return [NSSet setWithObjects:@"cropTop", nil];
}

- (int)maxBottomCrop
{
    return self.sourceHeight - self.cropTop - 32;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxLeftCrop
{
    return [NSSet setWithObjects:@"cropRight", nil];
}

- (int)maxLeftCrop
{
    return self.sourceWidth - self.cropRight - 32;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxRightCrop
{
    return [NSSet setWithObjects:@"cropLeft", nil];
}

- (int)maxRightCrop
{
    return self.sourceWidth - self.cropLeft - 32;
}

- (int)sourceDisplayWidth
{
    return (int) (self.sourceWidth * self.sourceParNum / (double)self.sourceParDen);
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

        uiGeo.mode = (int)self.anamorphicMode;
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
    encodeInteger(_anamorphicMode);
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

    decodeInt(_width); if (_width < 0) { goto fail; }
    decodeInt(_height); if (_height < 0) { goto fail; }

    decodeBool(_keepDisplayAspect);
    decodeInteger(_anamorphicMode);
    if (_anamorphicMode < HBPictureAnarmophicModeNone || _anamorphicMode > HBPictureAnarmophicModeAuto)
    {
        goto fail;
    }

    decodeInt(_modulus); if (_modulus < 2 || _modulus > 16) { goto fail; }

    decodeInt(_displayWidth); if (_displayWidth < 0) { goto fail; }
    decodeInt(_parWidth); if (_parWidth < 0) { goto fail; }
    decodeInt(_parHeight); if (_parHeight < 0) { goto fail; }

    decodeBool(_autocrop);
    decodeInt(_cropTop); if (_cropTop < 0) { goto fail; }
    decodeInt(_cropBottom); if (_cropBottom < 0) { goto fail; }
    decodeInt(_cropLeft); if (_cropLeft < 0) { goto fail; }
    decodeInt(_cropRight); if (_cropRight < 0) { goto fail; }

    decodeInt(_autoCropTop); if (_autoCropTop < 0) { goto fail; }
    decodeInt(_autoCropBottom); if (_autoCropBottom < 0) { goto fail; }
    decodeInt(_autoCropLeft); if (_autoCropLeft < 0) { goto fail; }
    decodeInt(_autoCropRight); if (_autoCropRight < 0) { goto fail; }

    decodeInt(_sourceWidth); if (_sourceWidth < 0) { goto fail; }
    decodeInt(_sourceHeight); if (_sourceHeight < 0) { goto fail; }
    decodeInt(_sourceParNum); if (_sourceParNum < 0) { goto fail; }
    decodeInt(_sourceParDen); if (_sourceParDen < 0) { goto fail; }

    _notificationsEnabled = YES;

    return self;

fail:
    return nil;
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
        case HB_ANAMORPHIC_AUTO:
            preset[@"PicturePAR"] = @"auto";
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

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    NSDictionary<NSString *, NSNumber *> *par = settings[@"PAR"];
    NSDictionary<NSString *, id> *filterList = settings[@"Filters"][@"FilterList"];
    NSDictionary<NSString *, NSNumber *> *cropScale = nil;

    for (NSDictionary *dict in filterList)
    {
        if ([dict[@"ID"] intValue] == HB_FILTER_CROP_SCALE)
        {
            cropScale = dict[@"Settings"];
        }
    }

    self.validating = YES;
    self.notificationsEnabled = NO;


    // Check to see if UsesPictureSettings is greater than 0, as 0 means use picture sizing "None"
    // (2 is use max for source and 1 is use exact size when the preset was created) and the
    // preset completely ignores any picture sizing values in the preset.
    if (cropScale && [preset[@"UsesPictureSettings"] intValue])
    {
        // If Cropping is set to custom, then recall all four crop values from
        // when the preset was created and apply them
        if ([preset[@"PictureAutoCrop"] boolValue])
        {
            self.autocrop = YES;

            // Here we use the auto crop values determined right after scan
            self.cropTop    = [cropScale[@"crop-top"] intValue];
            self.cropBottom = [cropScale[@"crop-bottom"] intValue];
            self.cropLeft   = [cropScale[@"crop-left"] intValue];
            self.cropRight  = [cropScale[@"crop-right"] intValue];
        }
        else
        {
            self.autocrop = NO;

            // Here we use the custom crop values saved at the time the preset was saved
            self.cropTop    = [preset[@"PictureTopCrop"] intValue];
            self.cropBottom = [preset[@"PictureBottomCrop"] intValue];
            self.cropLeft   = [preset[@"PictureLeftCrop"] intValue];
            self.cropRight  = [preset[@"PictureRightCrop"] intValue];
        }

        // Set modulus
        if (preset[@"PictureModulus"])
        {
            self.modulus = [preset[@"PictureModulus"]  intValue];
        }
        else
        {
            self.modulus = 16;
        }

        if (self.modulus <= 0 || self.modulus > 16)
        {
            self.modulus = 2;
        }

        // Assume max picture settings initially.
        self.keepDisplayAspect = [preset[@"PictureKeepRatio"] boolValue];

        if ([preset[@"PicturePAR"] isEqualToString:@"off"])
        {
            self.anamorphicMode = HB_ANAMORPHIC_NONE;
        }
        else if ([preset[@"PicturePAR"] isEqualToString:@"auto"])
        {
            self.anamorphicMode = HB_ANAMORPHIC_AUTO;
        }
        else if ([preset[@"PicturePAR"] isEqualToString:@"custom"])
        {
            self.anamorphicMode = HB_ANAMORPHIC_CUSTOM;
        }
        else
        {
            self.anamorphicMode = HB_ANAMORPHIC_LOOSE;
        }

        self.parWidth = [par[@"Num"] intValue];
        self.parHeight = [par[@"Den"] intValue];

        self.width = [cropScale[@"width"] intValue];
        self.height = [cropScale[@"height"] intValue];

        self.displayWidth = self.width * self.parWidth / self.parHeight;
    }

    self.validating = NO;
    self.notificationsEnabled = YES;

    [self postChangedNotification];
}

@end
