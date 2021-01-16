/*  HBPicture.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPicture.h"
#import "HBTitle.h"

#import "HBCodingUtilities.h"
#import "HBMutablePreset.h"
#import "NSDictionary+HBAdditions.h"

#include "handbrake/handbrake.h"

NSString * const HBPictureChangedNotification = @"HBPictureChangedNotification";

@interface HBPicture ()

@property (nonatomic, readwrite, getter=isValidating) BOOL validating;
@property (nonatomic, readwrite, getter=areNotificationsEnabled) BOOL notificationsEnabled;

@property (nonatomic, readonly) int modulus;

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
        _maxWidth = 1920;
        _maxHeight = 1080;

        _width = 1920;
        _height = 1080;

        _sourceWidth = 1920;
        _sourceHeight = 1080;

        _anamorphicMode = HBPictureAnarmophicModeNone;

        _paddingMode = HBPicturePaddingModeNone;
        _paddingColorMode = HBPicturePaddingColorModeBlack;
        _paddingColorCustom = @"";

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
        [NSNotificationCenter.defaultCenter postNotification:[NSNotification notificationWithName:HBPictureChangedNotification
                                                                                              object:self
                                                                                            userInfo:nil]];
    }
}

#pragma mark - Rotate

- (void)setRotate:(int)rotate
{
    if (rotate != _rotate)
    {
        [[self.undo prepareWithInvocationTarget:self] setRotate:_rotate];
    }
    _rotate = rotate;
    [self postChangedNotification];
}

#pragma mark - Flip

- (void)setFlip:(BOOL)flip
{
    if (flip != _flip)
    {
        [[self.undo prepareWithInvocationTarget:self] setFlip:_flip];
    }
    _flip = flip;
    [self postChangedNotification];
}

#pragma mark - Resolution limit

- (void)setResolutionLimitMode:(HBPictureResolutionLimitMode)resolutionLimit
{
    if (resolutionLimit != _resolutionLimitMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setResolutionLimitMode:_resolutionLimitMode];
    }
    _resolutionLimitMode = resolutionLimit;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        switch (resolutionLimit) {
            case HBPictureResolutionLimitModeNone:
                self.maxWidth = HB_MAX_WIDTH;
                self.maxHeight = HB_MAX_HEIGHT;
                break;
            case HBPictureResolutionLimitMode8K:
                self.maxWidth = 7680;
                self.maxHeight = 4320;
                break;
            case HBPictureResolutionLimitMode4K:
                self.maxWidth = 3840;
                self.maxHeight = 2160;
                break;
            case HBPictureResolutionLimitMode1440p:
                self.maxWidth = 2560;
                self.maxHeight = 1440;
                break;
            case HBPictureResolutionLimitMode1080p:
                self.maxWidth = 1920;
                self.maxHeight = 1080;
                break;
            case HBPictureResolutionLimitMode720p:
                self.maxWidth = 1280;
                self.maxHeight = 720;
                break;
            case HBPictureResolutionLimitMode576p:
                self.maxWidth = 720;
                self.maxHeight = 576;
                break;
            case HBPictureResolutionLimitMode480p:
                self.maxWidth = 720;
                self.maxHeight = 480;
                break;
            case HBPictureResolutionLimitModeCustom:
                break;
        }
    }

    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setMaxWidth:(int)maxWidth
{
    if (maxWidth != _maxWidth)
    {
        [[self.undo prepareWithInvocationTarget:self] setMaxWidth:_maxWidth];
    }
    _maxWidth = maxWidth;

    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (BOOL)validateMaxWidth:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];

        if (value >= HB_MAX_WIDTH)
        {
            *ioValue = @(HB_MAX_WIDTH);
        }
        else if (value <= HB_MIN_WIDTH)
        {
            *ioValue = @(HB_MIN_WIDTH);
        }
    }

    return retval;
}

- (void)setMaxHeight:(int)maxHeight
{
    if (maxHeight != _maxHeight)
    {
        [[self.undo prepareWithInvocationTarget:self] setMaxHeight:_maxHeight];
    }
    _maxHeight = maxHeight;

    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (BOOL)validateMaxHeight:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];

        if (value >= HB_MAX_HEIGHT)
        {
            *ioValue = @(HB_MAX_HEIGHT);
        }
        else if (value <= HB_MIN_HEIGHT)
        {
            *ioValue = @(HB_MIN_HEIGHT);
        }
    }

    return retval;
}

- (void)setAllowUpscaling:(BOOL)allowUpscaling
{
    if (allowUpscaling != _allowUpscaling)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowUpscaling:_allowUpscaling];
    }
    _allowUpscaling = allowUpscaling;

    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setUseMaximumSize:(BOOL)useMaximumSize
{
    if (useMaximumSize != _useMaximumSize)
    {
        [[self.undo prepareWithInvocationTarget:self] setUseMaximumSize:_useMaximumSize];
    }
    _useMaximumSize = useMaximumSize;

    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

#pragma mark - Size

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

        if (!self.allowUpscaling && value >= self.sourceWidth)
        {
            *ioValue = @(self.sourceWidth);
        }
        else if (value >= self.maxWidth)
        {
            *ioValue = @(self.maxWidth);
        }
        else if (value <= HB_MIN_WIDTH)
        {
            *ioValue = @(HB_MIN_WIDTH);
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

        if (!self.allowUpscaling && value >= self.sourceHeight)
        {
            *ioValue = @(self.sourceHeight);
        }
        else if (value >= self.maxHeight)
        {
            *ioValue = @(self.maxHeight);
        }
        else if (value <= HB_MIN_HEIGHT)
        {
            *ioValue = @(HB_MIN_HEIGHT);
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

#pragma mark - Crop

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

#pragma mark - Padding

- (void)setPaddingTop:(int)paddingTop
{
    if (paddingTop != _paddingTop)
    {
        [[self.undo prepareWithInvocationTarget:self] setPaddingTop:_paddingTop];
    }
    _paddingTop = paddingTop;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setPaddingBottom:(int)paddingBottom
{
    if (paddingBottom != _paddingBottom)
    {
        [[self.undo prepareWithInvocationTarget:self] setPaddingBottom:_paddingBottom];
    }
    _paddingBottom = paddingBottom;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setPaddingLeft:(int)paddingLeft
{
    if (paddingLeft != _paddingLeft)
    {
        [[self.undo prepareWithInvocationTarget:self] setPaddingLeft:_paddingLeft];
    }
    _paddingLeft = paddingLeft;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setPaddingRight:(int)paddingRight
{
    if (paddingRight != _paddingRight)
    {
        [[self.undo prepareWithInvocationTarget:self] setPaddingRight:_paddingRight];
    }
    _paddingRight = paddingRight;
    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (BOOL)validatePaddingTop:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validatePadding:ioValue max:self.maxTopPadding];
    return YES;
}

- (BOOL)validatePaddingBottom:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validatePadding:ioValue max:self.maxBottomPadding];
    return YES;
}

- (BOOL)validatePaddingLeft:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validatePadding:ioValue max:self.maxLeftPadding];
    return YES;
}

- (BOOL)validatePaddingRight:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validatePadding:ioValue max:self.maxRightPadding];
    return YES;
}

- (void)validatePadding:(NSNumber **)ioValue  max:(int)maxPadding
{
    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];
        if (value >= maxPadding)
        {
            *ioValue =  @(maxPadding);
        }
        else if (value < 0)
        {
            *ioValue = @0;
        }
    }
}

- (void)setPaddingMode:(HBPicturePaddingMode)paddingMode
{
    if (paddingMode != _paddingMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setPaddingMode:_paddingMode];
    }
    _paddingMode = paddingMode;

    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

- (void)setPaddingColorMode:(HBPicturePaddingColorMode)paddingColorMode
{
    if (paddingColorMode != _paddingColorMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setPaddingColorMode:_paddingColorMode];
    }
    _paddingColorMode = paddingColorMode;

    if (!self.isValidating)
    {
        [self validateSettings];
    }
}

#pragma mark - Anamorphic

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

- (int)modulus
{
    return 2;
}

#pragma mark - Max sizes

- (int)maxTopCrop
{
    return self.sourceHeight - self.cropBottom - HB_MIN_HEIGHT;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxBottomCrop
{
    return [NSSet setWithObjects:@"cropTop", nil];
}

- (int)maxBottomCrop
{
    return self.sourceHeight - self.cropTop - HB_MIN_HEIGHT;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxLeftCrop
{
    return [NSSet setWithObjects:@"cropRight", nil];
}

- (int)maxLeftCrop
{
    return self.sourceWidth - self.cropRight - HB_MIN_WIDTH;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxRightCrop
{
    return [NSSet setWithObjects:@"cropLeft", nil];
}

- (int)maxRightCrop
{
    return self.sourceWidth - self.cropLeft - HB_MIN_WIDTH;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxTopPadding
{
    return [NSSet setWithObjects:@"paddingBottom, maxHeight, height", nil];
}

- (int)maxTopPadding
{
    return self.maxHeight - self.height - self.paddingBottom;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxBottomPadding
{
    return [NSSet setWithObjects:@"paddingTop, maxHeight, height", nil];
}

- (int)maxBottomPadding
{
    return self.maxHeight - self.height - self.paddingTop;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxLeftPadding
{
    return [NSSet setWithObjects:@"paddingRight, maxWidth, width", nil];
}

- (int)maxLeftPadding
{
    return self.maxWidth - self.width - self.paddingRight;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxRightPadding
{
    return [NSSet setWithObjects:@"paddingLeft, maxWidth, width", nil];
}

- (int)maxRightPadding
{
    return self.maxWidth - self.width - self.paddingLeft;
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
        uiGeo.geometry.width = self.useMaximumSize || self.maxWidth < self.width ? self.maxWidth : self.width;
        uiGeo.geometry.height =  self.useMaximumSize || self.maxHeight < self.height ? self.maxHeight : self.height;
        // Modulus added to maxWidth/maxHeight to allow a small amount of
        // upscaling to the next mod boundary.
        uiGeo.maxWidth = self.allowUpscaling || self.maxWidth < self.width ? self.maxWidth : self.sourceWidth - crop[2] - crop[3] + self.modulus - 1;
        uiGeo.maxHeight = self.allowUpscaling || self.maxHeight < self.height ? self.maxHeight : self.sourceHeight - crop[0] - crop[1] + self.modulus - 1;

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

        int maxPaddingX = self.maxWidth - self.width;
        int maxPaddingY = self.maxHeight - self.height;

        switch (self.paddingMode)
        {
            case HBPicturePaddingModeNone:
                self.paddingRight   = 0;
                self.paddingLeft    = 0;
                self.paddingTop     = 0;
                self.paddingBottom  = 0;
                break;
            case HBPicturePaddingModeFill:
                self.paddingRight   = maxPaddingX / 2;
                self.paddingLeft    = maxPaddingX / 2;
                self.paddingTop     = maxPaddingY / 2;
                self.paddingBottom  = maxPaddingY / 2;
                break;
            case HBPicturePaddingModeFillHeight:
                self.paddingRight   = 0;
                self.paddingLeft    = 0;
                self.paddingTop     = maxPaddingY / 2;
                self.paddingBottom  = maxPaddingY / 2;
                break;
            case HBPicturePaddingModeFillWidth:
                self.paddingRight   = maxPaddingX / 2;
                self.paddingLeft    = maxPaddingX / 2;
                self.paddingTop     = 0;
                self.paddingBottom  = 0;
                break;
            case HBPicturePaddingModeCustom:
                break;
        }
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
        copy->_rotate = _rotate;
        copy->_flip = _flip;

        copy->_resolutionLimitMode = _resolutionLimitMode;
        copy->_maxWidth = _maxWidth;
        copy->_maxHeight = _maxHeight;
        copy->_allowUpscaling = _allowUpscaling;
        copy->_useMaximumSize = _useMaximumSize;

        copy->_width = _width;
        copy->_height = _height;

        copy->_keepDisplayAspect = _keepDisplayAspect;
        copy->_anamorphicMode = _anamorphicMode;

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

        copy->_paddingMode = _paddingMode;
        copy->_paddingTop = _paddingTop;
        copy->_paddingBottom = _paddingBottom;
        copy->_paddingLeft = _paddingLeft;
        copy->_paddingRight = _paddingRight;
        copy->_paddingColorMode = _paddingColorMode;
        copy->_paddingColorCustom = [_paddingColorCustom copy];

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
    [coder encodeInt:2 forKey:@"HBPictureVersion"];

    encodeInt(_sourceWidth);
    encodeInt(_sourceHeight);
    encodeInt(_sourceParNum);
    encodeInt(_sourceParDen);

    encodeInt(_rotate);
    encodeBool(_flip);

    encodeInteger(_resolutionLimitMode);
    encodeInt(_maxWidth);
    encodeInt(_maxHeight);
    encodeBool(_allowUpscaling);
    encodeBool(_useMaximumSize);

    encodeInt(_width);
    encodeInt(_height);

    encodeBool(_keepDisplayAspect);
    encodeInteger(_anamorphicMode);

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

    encodeInteger(_paddingMode);
    encodeInt(_paddingTop);
    encodeInt(_paddingBottom);
    encodeInt(_paddingLeft);
    encodeInt(_paddingRight);
    encodeInteger(_paddingColorMode);
    encodeObject(_paddingColorCustom);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_sourceWidth); if (_sourceWidth < 0) { goto fail; }
    decodeInt(_sourceHeight); if (_sourceHeight < 0) { goto fail; }
    decodeInt(_sourceParNum); if (_sourceParNum < 0) { goto fail; }
    decodeInt(_sourceParDen); if (_sourceParDen < 0) { goto fail; }

    decodeInt(_rotate); if (_rotate != 0 && _rotate != 90 && _rotate != 180 && _rotate != 270) { goto fail; }
    decodeBool(_flip);

    decodeInteger(_resolutionLimitMode);
    if (_resolutionLimitMode < HBPictureResolutionLimitModeNone || _resolutionLimitMode > HBPictureResolutionLimitModeCustom)
    {
        goto fail;
    }
    decodeInt(_maxWidth); if (_maxWidth < 0 || _maxWidth > HB_MAX_WIDTH) { goto fail; }
    decodeInt(_maxHeight); if (_maxHeight < 0 || _maxHeight > HB_MAX_HEIGHT) { goto fail; }
    decodeBool(_allowUpscaling);
    decodeBool(_useMaximumSize);

    decodeInt(_width); if (_width < 0 || _maxWidth > HB_MAX_WIDTH) { goto fail; }
    decodeInt(_height); if (_height < 0 || _maxHeight > HB_MAX_HEIGHT) { goto fail; }

    decodeBool(_keepDisplayAspect);
    decodeInteger(_anamorphicMode);
    if (_anamorphicMode < HBPictureAnarmophicModeNone || _anamorphicMode > HBPictureAnarmophicModeAuto)
    {
        goto fail;
    }

    decodeInt(_displayWidth); if (_displayWidth < 0) { goto fail; }
    decodeInt(_parWidth); if (_parWidth < 0) { goto fail; }
    decodeInt(_parHeight); if (_parHeight < 0) { goto fail; }

    decodeBool(_autocrop);
    decodeInt(_cropTop); if (_cropTop < 0 || _cropTop > _sourceHeight) { goto fail; }
    decodeInt(_cropBottom); if (_cropBottom < 0 || _cropBottom > _sourceHeight) { goto fail; }
    decodeInt(_cropLeft); if (_cropLeft < 0 || _cropLeft > _sourceWidth) { goto fail; }
    decodeInt(_cropRight); if (_cropRight < 0 || _cropRight > _sourceWidth) { goto fail; }

    decodeInt(_autoCropTop); if (_autoCropTop < 0 || _autoCropTop > _sourceHeight) { goto fail; }
    decodeInt(_autoCropBottom); if (_autoCropBottom < 0 || _autoCropBottom > _sourceHeight) { goto fail; }
    decodeInt(_autoCropLeft); if (_autoCropLeft < 0 || _autoCropLeft > _sourceWidth) { goto fail; }
    decodeInt(_autoCropRight); if (_autoCropRight < 0 || _autoCropLeft > _sourceWidth) { goto fail; }

    decodeInteger(_paddingMode);
    if (_paddingMode < HBPicturePaddingModeNone || _paddingMode > HBPicturePaddingModeCustom)
    {
        goto fail;
    }
    decodeInt(_paddingTop); if (_paddingTop < 0 || _paddingTop > HB_MAX_HEIGHT) { goto fail; }
    decodeInt(_paddingBottom); if (_paddingBottom < 0 || _paddingBottom > HB_MAX_HEIGHT) { goto fail; }
    decodeInt(_paddingLeft); if (_paddingLeft < 0 || _paddingLeft > HB_MAX_WIDTH) { goto fail; }
    decodeInt(_paddingRight); if (_paddingRight < 0 || _paddingRight > HB_MAX_WIDTH) { goto fail; }
    decodeInteger(_paddingColorMode);
    if (_paddingColorMode < HBPicturePaddingColorModeBlack || _paddingColorMode > HBPicturePaddingColorModeCustom)
    {
        goto fail;
    }
    decodeObject(_paddingColorCustom, NSString);

    _notificationsEnabled = YES;

    return self;

fail:
    return nil;
}

#pragma mark - Presets

- (void)writeToPreset:(HBMutablePreset *)preset
{
    preset[@"PictureRotate"] = [NSString stringWithFormat:@"angle=%d:hflip=%d", self.rotate, self.flip];

    preset[@"PictureWidth"] = @(self.maxWidth == HB_MAX_WIDTH ? 0 : self.maxWidth);
    preset[@"PictureHeight"] = @(self.maxHeight == HB_MAX_HEIGHT ? 0 : self.maxHeight);

    preset[@"PictureAllowUpscaling"] = @(self.allowUpscaling);
    preset[@"PictureUseMaximumSize"] = @(self.useMaximumSize);

    preset[@"PictureKeepRatio"] = @(self.keepDisplayAspect);

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

    // Padding
    preset[@"PicturePadMode"] = @(self.paddingMode);
    int width = self.width + self.paddingLeft + self.paddingRight;
    int height = self.height + self.paddingTop + self.paddingBottom;
    NSString *color = self.paddingColorCustom;
    preset[@"PicturePad"] = [NSString stringWithFormat:@"width=%d:height=%d:color=%@:x=%d:y=%d", width, height, color, self.paddingLeft, self.paddingTop];
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

    // Rotate
    NSString *rotate = preset[@"PictureRotate"];
    hb_dict_t *hbdict = hb_parse_filter_settings(rotate.UTF8String);
    NSDictionary *dict = [[NSDictionary alloc] initWithHBDict:hbdict];
    hb_value_free(&hbdict);

    self.rotate = [dict[@"angle"] intValue];
    self.flip = [dict[@"hflip"] boolValue];

    self.maxWidth = [preset[@"PictureWidth"] intValue];
    self.maxHeight = [preset[@"PictureHeight"] intValue];

    self.allowUpscaling = [preset[@"PictureAllowUpscaling"] boolValue];
    self.useMaximumSize = [preset[@"PictureUseMaximumSize"] boolValue];

    if (self.maxWidth == 0 && self.maxHeight == 0)
    {
        self.resolutionLimitMode = HBPictureResolutionLimitModeNone;
    }
    else if (self.maxWidth == 7680 && self.maxHeight == 4320)
    {
        self.resolutionLimitMode = HBPictureResolutionLimitMode8K;
    }
    else if (self.maxWidth == 3840 && self.maxHeight == 2160)
    {
        self.resolutionLimitMode = HBPictureResolutionLimitMode4K;
    }
    else if (self.maxWidth == 2560 && self.maxHeight == 1440)
    {
        self.resolutionLimitMode = HBPictureResolutionLimitMode1440p;
    }
    else if (self.maxWidth == 1920 && self.maxHeight == 1080)
    {
        self.resolutionLimitMode = HBPictureResolutionLimitMode1080p;
    }
    else if (self.maxWidth == 1280 && self.maxHeight == 720)
    {
        self.resolutionLimitMode = HBPictureResolutionLimitMode720p;
    }
    else if (self.maxWidth == 720 && self.maxHeight == 576)
    {
        self.resolutionLimitMode = HBPictureResolutionLimitMode576p;
    }
    else if (self.maxWidth == 720 && self.maxHeight == 480)
    {
        self.resolutionLimitMode = HBPictureResolutionLimitMode480p;
    }
    else
    {
        self.resolutionLimitMode = HBPictureResolutionLimitModeCustom;
    }

    if (cropScale)
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

        // Assume max picture settings initially.
        self.keepDisplayAspect = [preset[@"PictureKeepRatio"] boolValue];

        if ([preset[@"PicturePAR"] isEqualToString:@"off"])
        {
            self.anamorphicMode = (HBPictureAnarmophicMode)HB_ANAMORPHIC_NONE;
        }
        else if ([preset[@"PicturePAR"] isEqualToString:@"auto"])
        {
            self.anamorphicMode = (HBPictureAnarmophicMode)HB_ANAMORPHIC_AUTO;
        }
        else if ([preset[@"PicturePAR"] isEqualToString:@"custom"])
        {
            self.anamorphicMode = (HBPictureAnarmophicMode)HB_ANAMORPHIC_CUSTOM;
        }
        else
        {
            self.anamorphicMode = (HBPictureAnarmophicMode)HB_ANAMORPHIC_LOOSE;
        }

        self.parWidth = [par[@"Num"] intValue];
        self.parHeight = [par[@"Den"] intValue];

        self.width = [cropScale[@"width"] intValue];
        self.height = [cropScale[@"height"] intValue];

        self.displayWidth = self.width * self.parWidth / self.parHeight;
    }

    // Padding
    NSString *pad = preset[@"PicturePad"];
    hb_dict_t *hbPadDict = hb_parse_filter_settings(pad.UTF8String);
    NSDictionary *padDict = [[NSDictionary alloc] initWithHBDict:hbPadDict];
    hb_value_free(&hbPadDict);

    self.paddingMode = [preset[@"PicturePadMode"] intValue];
    if (self.paddingMode == HBPicturePaddingModeCustom)
    {
        self.paddingLeft    = [padDict[@"x"] intValue];
        self.paddingRight   = [padDict[@"width"] intValue] - self.width - self.paddingLeft;
        self.paddingTop     = [padDict[@"y"] intValue];
        self.paddingBottom  = [padDict[@"height"] intValue] - self.height - self.paddingTop;
    }
    else
    {
        self.paddingLeft   = 0;
        self.paddingRight  = 0;
        self.paddingTop    = 0;
        self.paddingBottom = 0;
    }

    self.validating = NO;
    self.notificationsEnabled = YES;

    [self postChangedNotification];
}

@end
