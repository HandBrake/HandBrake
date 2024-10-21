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

@property (nonatomic, readwrite, getter=isUpdating) BOOL updating;
@property (nonatomic, readwrite, getter=areNotificationsEnabled) BOOL notificationsEnabled;

@property (nonatomic, readonly) int modulus;

@property (nonatomic, readonly) int autoCropTop;
@property (nonatomic, readonly) int autoCropBottom;
@property (nonatomic, readonly) int autoCropLeft;
@property (nonatomic, readonly) int autoCropRight;

@property (nonatomic, readonly) int looseAutoCropTop;
@property (nonatomic, readonly) int looseAutoCropBottom;
@property (nonatomic, readonly) int looseAutoCropLeft;
@property (nonatomic, readonly) int looseAutoCropRight;

@property (nonatomic, readwrite) int storageWidth;
@property (nonatomic, readwrite) int storageHeight;

@end

@implementation HBPicture

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        // Set some values if we ever need a fake instance
        _sourceWidth = 1920;
        _sourceHeight = 1080;
        _sourceParNum = 1;
        _sourceParDen = 1;
        _maxWidth = 1920;
        _maxHeight = 1080;
        _parNum = 1;
        _parDen = 1;
        _width = 1920;
        _height = 1080;
        _storageWidth = 1920;
        _storageHeight = 1080;
        _keepAspectRatio = YES;
        _displayWidth = 1920;
        _displayHeight = 1080;
        _anamorphicMode = HBPictureAnarmophicModeNone;

        _padMode = HBPicturePadModeNone;
        _padColorMode = HBPicturePadColorModeBlack;
        _padColorCustom = @"";
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

        _looseAutoCropTop    = title.looseAutoCropTop;
        _looseAutoCropBottom = title.looseAutoCropBottom;
        _looseAutoCropLeft   = title.looseAutoCropLeft;
        _looseAutoCropRight  = title.looseAutoCropRight;

        [self updatePictureSettings:0];

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

- (void)setAngle:(int)angle
{
    if (angle != _angle)
    {
        [[self.undo prepareWithInvocationTarget:self] setAngle:_angle];
    }
    int prev_angle = _angle;
    _angle = angle;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self applyRotation:prev_angle flip:self.flip];
    }

    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }

    [self postChangedNotification];
}

#pragma mark - Flip

- (void)setFlip:(BOOL)flip
{
    if (flip != _flip)
    {
        [[self.undo prepareWithInvocationTarget:self] setFlip:_flip];
    }
    int prev_flip = _flip;
    _flip = flip;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self applyRotation:self.angle flip:prev_flip];
    }

    if (!self.isUpdating)
    {
        [self updatePictureSettings:HB_KEEP_WIDTH];
    }

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
                self.maxWidth = 0;
                self.maxHeight = 0;
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

    if (!self.isUpdating)
    {
        [self updatePictureSettings:HB_KEEP_HEIGHT];
    }
}

- (void)setMaxWidth:(int)maxWidth
{
    if (maxWidth != _maxWidth)
    {
        [[self.undo prepareWithInvocationTarget:self] setMaxWidth:_maxWidth];
    }
    _maxWidth = maxWidth;

    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
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
            value = HB_MAX_WIDTH;
        }
        else if (value <= HB_MIN_WIDTH)
        {
            value = HB_MIN_WIDTH;
        }

        *ioValue = @(MULTIPLE_MOD_DOWN(value, self.modulus));
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

    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
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
            value = HB_MAX_HEIGHT;
        }
        else if (value <= HB_MIN_HEIGHT)
        {
            value = HB_MIN_HEIGHT;
        }

        *ioValue = @(MULTIPLE_MOD_DOWN(value, self.modulus));
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

    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setUseMaximumSize:(BOOL)useMaximumSize
{
    if (useMaximumSize != _useMaximumSize)
    {
        [[self.undo prepareWithInvocationTarget:self] setUseMaximumSize:_useMaximumSize];
    }
    _useMaximumSize = useMaximumSize;

    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
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

    if (!self.isUpdating)
    {
        [self updatePictureSettings:HB_KEEP_WIDTH];
    }
}

- (BOOL)validateWidth:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        hb_geometry_t resultGeo;
        hb_geometry_settings_t uiGeo;
        bzero(&uiGeo, sizeof(uiGeo));

        int value = [*ioValue intValue];
        uiGeo.geometry.width = value;

        [self calculatePictureSettings:&resultGeo uiGeo:&uiGeo mode:HB_KEEP_WIDTH];

        if (value != resultGeo.width)
        {
            *ioValue = @(resultGeo.width);
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
    if (!self.isUpdating)
    {
        [self updatePictureSettings:HB_KEEP_HEIGHT];
    }
}

- (BOOL)validateHeight:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        hb_geometry_t resultGeo;
        hb_geometry_settings_t uiGeo;
        bzero(&uiGeo, sizeof(uiGeo));

        int value = [*ioValue intValue];
        uiGeo.geometry.height = value;

        [self calculatePictureSettings:&resultGeo uiGeo:&uiGeo mode:HB_KEEP_HEIGHT];

        if (value != resultGeo.height)
        {
            *ioValue = @(resultGeo.height);
        }
    }

    return retval;
}

- (void)setParNum:(int)parWidth
{
    if (parWidth != _parNum)
    {
        [[self.undo prepareWithInvocationTarget:self] setParNum:_parNum];
    }
    _parNum = parWidth;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:HB_KEEP_HEIGHT];
    }
}

- (BOOL)validateParNum:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];

        if (value < 1)
        {
            value = 1;
        }
        else if (value > UINT16_MAX)
        {
            value = UINT16_MAX;
        }

        *ioValue = @(value);
    }

    return retval;
}

- (void)setParDen:(int)parHeight
{
    if (parHeight != _parDen)
    {
        [[self.undo prepareWithInvocationTarget:self] setParDen:_parDen];
    }
    _parDen = parHeight;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:HB_KEEP_HEIGHT];
    }
}

- (BOOL)validateParDen:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];

        if (value < 1)
        {
            value = 1;
        }
        else if (value > UINT16_MAX)
        {
            value = UINT16_MAX;
        }

        *ioValue = @(value);
    }

    return retval;
}

#pragma mark - Crop

- (void)setCropTop:(int)cropTop
{
    if (cropTop != _cropTop)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropTop:_cropTop];
    }
    _cropTop = cropTop;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setCropBottom:(int)cropBottom
{
    if (cropBottom != _cropBottom)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropBottom:_cropBottom];
    }
    _cropBottom = cropBottom;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setCropLeft:(int)cropLeft
{
    if (cropLeft != _cropLeft)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropLeft:_cropLeft];
    }
    _cropLeft = cropLeft;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setCropRight:(int)cropRight
{
    if (cropRight != _cropRight)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropRight:_cropRight];
    }
    _cropRight = cropRight;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
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
            value =  maxCrop;
        }
        else if (value < 0)
        {
            value = 0;
        }

        *ioValue = @(MULTIPLE_MOD_DOWN(value, self.modulus));
    }
}

- (void)setCropMode:(HBPictureCropMode)cropMode
{
    if (cropMode != _cropMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setCropMode:_cropMode];
    }
    _cropMode = cropMode;

    if (cropMode != HBPictureCropModeCustom && !self.isUpdating)
    {
        if (!(self.undo.isUndoing || self.undo.isRedoing))
        {
            self.updating = YES;
            if (cropMode == HBPictureCropModeAutomatic ||
                cropMode == HBPictureCropModeConservative)
            {
                // Reset the crop values to those determined right after scan
                hb_geometry_crop_t geo = {0,};
                if (cropMode == HBPictureCropModeAutomatic)
                {
                    geo.crop[0] = self.autoCropTop;
                    geo.crop[1] = self.autoCropBottom;
                    geo.crop[2] = self.autoCropLeft;
                    geo.crop[3] = self.autoCropRight;
                }
                else
                {
                    geo.crop[0] = self.looseAutoCropTop;
                    geo.crop[1] = self.looseAutoCropBottom;
                    geo.crop[2] = self.looseAutoCropLeft;
                    geo.crop[3] = self.looseAutoCropRight;
                }
                hb_rotate_geometry(&geo, &geo, self.angle, self.flip);
                self.cropTop    = geo.crop[0];
                self.cropBottom = geo.crop[1];
                self.cropLeft   = geo.crop[2];
                self.cropRight  = geo.crop[3];
                self.updating = NO;
            }
            else if (cropMode == HBPictureCropModeNone)
            {
                self.cropTop    = 0;
                self.cropBottom = 0;
                self.cropLeft   = 0;
                self.cropRight  = 0;
            }
        }
        [self updatePictureSettings:0];
    }
}

#pragma mark - Pad

- (void)setPadTop:(int)padTop
{
    if (padTop != _padTop)
    {
        [[self.undo prepareWithInvocationTarget:self] setPadTop:_padTop];
    }
    _padTop = padTop;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setPadBottom:(int)padBottom
{
    if (padBottom != _padBottom)
    {
        [[self.undo prepareWithInvocationTarget:self] setPadBottom:_padBottom];
    }
    _padBottom = padBottom;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setPadLeft:(int)padLeft
{
    if (padLeft != _padLeft)
    {
        [[self.undo prepareWithInvocationTarget:self] setPadLeft:_padLeft];
    }
    _padLeft = padLeft;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setPadRight:(int)padRight
{
    if (padRight != _padRight)
    {
        [[self.undo prepareWithInvocationTarget:self] setPadRight:_padRight];
    }
    _padRight = padRight;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (BOOL)validatePadTop:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validatePad:ioValue max:self.maxTopPad];
    return YES;
}

- (BOOL)validatePadBottom:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validatePad:ioValue max:self.maxBottomPad];
    return YES;
}

- (BOOL)validatePadLeft:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validatePad:ioValue max:self.maxLeftPad];
    return YES;
}

- (BOOL)validatePadRight:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    [self validatePad:ioValue max:self.maxRightPad];
    return YES;
}

- (void)validatePad:(NSNumber **)ioValue  max:(int)maxPad
{
    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];
        if (value >= maxPad)
        {
            value =  maxPad;
        }
        else if (value < 0)
        {
            value = 0;
        }

        *ioValue = @(MULTIPLE_MOD_DOWN(value, self.modulus));
    }
}

- (void)setPadMode:(HBPicturePadMode)padMode
{
    if (padMode != _padMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setPadMode:_padMode];
    }
    _padMode = padMode;

    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setPadColorMode:(HBPicturePadColorMode)padColorMode
{
    if (padColorMode != _padColorMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setPadColorMode:_padColorMode];
    }
    _padColorMode = padColorMode;

    if (!self.isUpdating)
    {
        [self postChangedNotification];
    }
}

- (void)setPadColorCustom:(NSString *)padColorCustom
{
    if (padColorCustom != _padColorCustom)
    {
        [[self.undo prepareWithInvocationTarget:self] setPadColorCustom:_padColorCustom];
    }
    _padColorCustom = padColorCustom;

    if (!self.isUpdating)
    {
        [self postChangedNotification];
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

    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
    }
}

- (void)setKeepAspectRatio:(BOOL)keepAspectRatio
{
    if (keepAspectRatio != _keepAspectRatio)
    {
        [[self.undo prepareWithInvocationTarget:self] setKeepAspectRatio:_keepAspectRatio];
    }
    _keepAspectRatio = keepAspectRatio;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:0];
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

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxTopPad
{
    return [NSSet setWithObjects:@"padBottom, maxHeight, height", nil];
}

- (int)maxPadWidth
{
    return self.maxWidth > 0 ? self.maxWidth : HB_MAX_WIDTH;
}

- (int)maxPadHeight
{
    return self.maxHeight > 0 ? self.maxHeight : HB_MAX_HEIGHT;
}

- (int)maxTopPad
{
    return self.maxPadHeight - HB_MIN_HEIGHT - self.padBottom;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxBottomPad
{
    return [NSSet setWithObjects:@"padTop, maxHeight, height", nil];
}

- (int)maxBottomPad
{
    return self.maxPadHeight - HB_MIN_HEIGHT - self.padTop;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxLeftPad
{
    return [NSSet setWithObjects:@"padRight, maxWidth, width", nil];
}

- (int)maxLeftPad
{
    return self.maxPadWidth - HB_MIN_WIDTH - self.padRight;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingMaxRightPad
{
    return [NSSet setWithObjects:@"padLeft, maxWidth, width", nil];
}

- (int)maxRightPad
{
    return self.maxPadWidth - HB_MIN_WIDTH - self.padLeft;
}

- (int)sourceDisplayWidth
{
    return (int)(self.sourceWidth * self.sourceParNum / (double)self.sourceParDen);
}

static const char * padModeToString(HBPicturePadMode padMode)
{
    switch (padMode)
    {
        case HBPicturePadModeFill:
            return "fill";
            break;
        case HBPicturePadModeFillHeight:
            return "letterbox";
            break;
        case HBPicturePadModeFillWidth:
            return "pillarbox";
            break;
        case HBPicturePadModeCustom:
            return "custom";
            break;
        default:
            return "none";
            break;
    }
}

#pragma mark - Output sizes

- (void)setStorageWidth:(int)storageWidth
{
    if (storageWidth != _storageWidth)
    {
        [[self.undo prepareWithInvocationTarget:self] setStorageWidth:_storageWidth];
    }
    _storageWidth = storageWidth;
}

- (void)setStorageHeight:(int)storageHeight
{
    if (storageHeight != _storageHeight)
    {
        [[self.undo prepareWithInvocationTarget:self] setStorageHeight:_storageHeight];
    }
    _storageHeight = storageHeight;
}

- (void)setDisplayWidth:(int)displayWidth
{
    if (displayWidth != _displayWidth)
    {
        [[self.undo prepareWithInvocationTarget:self] setDisplayWidth:_displayWidth];
    }
    _displayWidth = displayWidth;
    if (!self.isUpdating)
    {
        [self updatePictureSettings:HB_KEEP_DISPLAY_WIDTH];
    }
}

- (BOOL)validateDisplayWidth:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        int value = [*ioValue intValue];
        if (value <= 0)
        {
            value = HB_MIN_WIDTH;
        }
        else if (value > HB_MAX_WIDTH)
        {
            value = HB_MAX_WIDTH;
        }

        hb_geometry_t resultGeo;
        hb_geometry_settings_t uiGeo;
        bzero(&uiGeo, sizeof(uiGeo));

        uiGeo.displayWidth = value;

        [self calculatePictureSettings:&resultGeo uiGeo:&uiGeo mode:HB_KEEP_DISPLAY_WIDTH];

        int displayWidth = ((double)resultGeo.par.num / resultGeo.par.den) * resultGeo.width + 0.5;

        if ([*ioValue intValue] != displayWidth)
        {
            *ioValue = @(displayWidth);
        }
    }

    return retval;
}

- (void)setDisplayHeight:(int)displayHeight
{
    if (displayHeight != _displayHeight)
    {
        [[self.undo prepareWithInvocationTarget:self] setDisplayHeight:_displayHeight];
    }
    _displayHeight = displayHeight;
}

- (void)setNilValueForKey:(NSString *)key
{
    if ([key isEqualToString:@"width"] || [key isEqualToString:@"height"])
    {
        [self setValue:@64 forKey:key];
    }
    else if ([key isEqualToString:@"parNum"] || [key isEqualToString:@"parDen"])
    {
        [self setValue:@1 forKey:key];
    }
    else
    {
        [self setValue:@0 forKey:key];
    }
}

#pragma mark - Picture Update Logic

- (void)applyRotation:(int)prev_angle flip:(BOOL)prev_hflip
{
    int angle, hflip;

    hb_geometry_crop_t   geo;

    // First normalize current settings to 0 angle 0 hflip
    geo.geometry.width   = self.width;
    geo.geometry.height  = self.height;
    geo.geometry.par.num = self.parNum;
    geo.geometry.par.den = self.parDen;
    geo.crop[0] = self.cropTop;
    geo.crop[1] = self.cropBottom;
    geo.crop[2] = self.cropLeft;
    geo.crop[3] = self.cropRight;

    geo.pad[0] = self.padTop;
    geo.pad[1] = self.padBottom;
    geo.pad[2] = self.padLeft;
    geo.pad[3] = self.padRight;
    // Normally hflip is applied, then rotation.
    // To revert, must apply rotation then hflip.
    hb_rotate_geometry(&geo, &geo, 360 - prev_angle, 0);
    hb_rotate_geometry(&geo, &geo, 0, prev_hflip);

    // If there is a title, reset to title width/height so that
    // dimension limits get properly re-applied
    geo.geometry.width = self.sourceWidth;
    geo.geometry.height = self.sourceHeight;

    // rotate dimensions to new angle and hflip
    angle = self.angle;
    hflip = self.flip;
    hb_rotate_geometry(&geo, &geo, angle, hflip);

    self.width = geo.geometry.width - geo.crop[2] - geo.crop[3];
    self.height = geo.geometry.height - geo.crop[2] - geo.crop[3];
    self.parNum = geo.geometry.par.num;
    self.parDen = geo.geometry.par.den;
    self.cropTop = geo.crop[0];
    self.cropBottom = geo.crop[1];
    self.cropLeft = geo.crop[2];
    self.cropRight = geo.crop[3];

    self.padTop = geo.pad[0];
    self.padBottom = geo.pad[1];
    self.padLeft = geo.pad[2];
    self.padRight = geo.pad[3];
}

- (void)applyPad:(const hb_geometry_settings_t *)geo result:(hb_geometry_t *)result
{
    bool fillwidth, fillheight;
    int pad[4] = {0,};

    fillwidth  = fillheight = self.padMode == HBPicturePadModeFill;
    fillheight = fillheight || (self.padMode == HBPicturePadModeFillHeight);
    fillwidth  = fillwidth  || (self.padMode == HBPicturePadModeFillWidth);

    if (self.padMode == HBPicturePadModeCustom)
    {
        pad[0] = self.padTop;
        pad[1] = self.padBottom;
        pad[2] = self.padLeft;
        pad[3] = self.padRight;
    }

    if (fillheight && geo->maxHeight > 0)
    {
        pad[0] = (geo->maxHeight - result->height) / 2;
        pad[1] =  geo->maxHeight - result->height - pad[0];
    }
    if (fillwidth && geo->maxWidth > 0)
    {
        pad[2] = (geo->maxWidth - result->width) / 2;
        pad[3] =  geo->maxWidth - result->width - pad[2];
    }

    self.padTop = pad[0];
    self.padBottom = pad[1];
    self.padLeft = pad[2];
    self.padRight = pad[3];

    result->width  += pad[2] + pad[3];
    result->height += pad[0] + pad[1];
}

- (void)calculatePictureSettings:(hb_geometry_t *)resultGeo uiGeo:(hb_geometry_settings_t *)uiGeo mode:(int)mode
{
    bool keep_aspect;
    bool autoscale, upscale;
    bool keep_width         = (mode & HB_KEEP_WIDTH);
    bool keep_height        = (mode & HB_KEEP_HEIGHT);
    bool keep_display_width = (mode & HB_KEEP_DISPLAY_WIDTH);
    const char *pad_mode;

    hb_geometry_crop_t srcGeo;

    srcGeo.geometry.width = self.sourceWidth;
    srcGeo.geometry.height = self.sourceHeight;
    srcGeo.geometry.par.num = self.sourceParNum;
    srcGeo.geometry.par.den = self.sourceParDen;
    srcGeo.crop[0] = self.autoCropTop;
    srcGeo.crop[1] = self.autoCropBottom;
    srcGeo.crop[2] = self.autoCropLeft;
    srcGeo.crop[3] = self.autoCropRight;

    // Rotate title dimensions so that they align with the current
    // orientation of dimensions tab settings
    hb_rotate_geometry(&srcGeo, &srcGeo, self.angle, self.flip);

    pad_mode    = padModeToString(self.padMode);
    autoscale   = self.useMaximumSize;
    upscale     = self.allowUpscaling;
    keep_aspect = self.keepAspectRatio;

    if (keep_display_width) { uiGeo->keep |= HB_KEEP_DISPLAY_WIDTH; }
    if (keep_width) { uiGeo->keep |= HB_KEEP_WIDTH; }
    if (keep_height) { uiGeo->keep |= HB_KEEP_HEIGHT; }
    if (keep_aspect) { uiGeo->keep |= HB_KEEP_DISPLAY_ASPECT; }
    if (!strcmp(pad_mode, "custom")) { uiGeo->keep |= HB_KEEP_PAD; }

    if (upscale) { uiGeo->flags |= HB_GEO_SCALE_UP; }
    if (autoscale) { uiGeo->flags |= HB_GEO_SCALE_BEST; }

    uiGeo->mode             = (int)self.anamorphicMode;
    uiGeo->modulus          = self.modulus;
    uiGeo->geometry.width   = uiGeo->geometry.width ? uiGeo->geometry.width : self.width;
    uiGeo->geometry.height  = uiGeo->geometry.height ? uiGeo->geometry.height : self.height;
    uiGeo->maxWidth         = self.maxWidth;
    uiGeo->maxHeight        = self.maxHeight;
    uiGeo->geometry.par.num = self.parNum;
    uiGeo->geometry.par.den = self.parDen;
    uiGeo->displayWidth     = uiGeo->displayWidth ? uiGeo->displayWidth : self.displayWidth;
    uiGeo->displayHeight    = self.displayHeight;

    uiGeo->pad[0] = self.padTop;
    uiGeo->pad[1] = self.padBottom;
    uiGeo->pad[2] = self.padLeft;
    uiGeo->pad[3] = self.padRight;

    uiGeo->crop[0] = self.cropTop;
    uiGeo->crop[1] = self.cropBottom;
    uiGeo->crop[2] = self.cropLeft;
    uiGeo->crop[3] = self.cropRight;

    // hb_set_anamorphic_size2 will adjust par, dar, and width/height
    // and enforce resolution limits
    hb_set_anamorphic_size2(&srcGeo.geometry, uiGeo, resultGeo);
}

/**
 *  Validates the settings through hb_set_anamorphic_size2,
 *  each setters calls this after setting its value.
 */
- (void)updatePictureSettings:(int)mode
{
    self.updating = YES;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        hb_geometry_t resultGeo;
        hb_geometry_settings_t uiGeo;
        bzero(&uiGeo, sizeof(uiGeo));

        [self calculatePictureSettings:&resultGeo uiGeo:&uiGeo mode:mode];

        self.width = resultGeo.width;
        self.height = resultGeo.height;

        self.parNum = resultGeo.par.num;
        self.parDen = resultGeo.par.den;

        uiGeo.maxWidth = self.maxWidth;
        uiGeo.maxHeight = self.maxHeight;
        [self applyPad:&uiGeo result:&resultGeo];

        self.displayWidth = ((double)resultGeo.par.num / resultGeo.par.den) * resultGeo.width + 0.5;
        self.displayHeight = resultGeo.height;

        self.storageWidth = resultGeo.width;
        self.storageHeight = resultGeo.height;
    }

    self.updating = NO;
    [self postChangedNotification];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBPicture *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_sourceWidth = _sourceWidth;
        copy->_sourceHeight = _sourceHeight;
        copy->_sourceParNum = _sourceParNum;
        copy->_sourceParDen = _sourceParDen;

        copy->_autoCropTop = _autoCropTop;
        copy->_autoCropBottom = _autoCropBottom;
        copy->_autoCropLeft = _autoCropLeft;
        copy->_autoCropRight = _autoCropRight;

        copy->_looseAutoCropTop = _looseAutoCropTop;
        copy->_looseAutoCropBottom = _looseAutoCropBottom;
        copy->_looseAutoCropLeft = _looseAutoCropLeft;
        copy->_looseAutoCropRight = _looseAutoCropRight;

        copy->_angle = _angle;
        copy->_flip = _flip;

        copy->_cropMode = _cropMode;
        copy->_cropTop = _cropTop;
        copy->_cropBottom = _cropBottom;
        copy->_cropLeft = _cropLeft;
        copy->_cropRight = _cropRight;

        copy->_resolutionLimitMode = _resolutionLimitMode;
        copy->_maxWidth = _maxWidth;
        copy->_maxHeight = _maxHeight;
        copy->_allowUpscaling = _allowUpscaling;
        copy->_useMaximumSize = _useMaximumSize;

        copy->_anamorphicMode = _anamorphicMode;
        copy->_parNum = _parNum;
        copy->_parDen = _parDen;
        copy->_width = _width;
        copy->_height = _height;

        copy->_padMode = _padMode;
        copy->_padTop = _padTop;
        copy->_padBottom = _padBottom;
        copy->_padLeft = _padLeft;
        copy->_padRight = _padRight;
        copy->_padColorMode = _padColorMode;
        copy->_padColorCustom = [_padColorCustom copy];

        copy->_storageWidth = _storageWidth;
        copy->_storageHeight = _storageHeight;
        copy->_keepAspectRatio = _keepAspectRatio;
        copy->_displayWidth = _displayWidth;
        copy->_displayHeight = _displayHeight;

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
    [coder encodeInt:4 forKey:@"HBPictureVersion"];

    encodeInt(_sourceWidth);
    encodeInt(_sourceHeight);
    encodeInt(_sourceParNum);
    encodeInt(_sourceParDen);

    encodeInt(_autoCropTop);
    encodeInt(_autoCropBottom);
    encodeInt(_autoCropLeft);
    encodeInt(_autoCropRight);

    encodeInt(_looseAutoCropTop);
    encodeInt(_looseAutoCropBottom);
    encodeInt(_looseAutoCropLeft);
    encodeInt(_looseAutoCropRight);

    encodeInt(_angle);
    encodeBool(_flip);

    encodeInteger(_cropMode);
    encodeInt(_cropTop);
    encodeInt(_cropBottom);
    encodeInt(_cropLeft);
    encodeInt(_cropRight);

    encodeInteger(_resolutionLimitMode);
    encodeInt(_maxWidth);
    encodeInt(_maxHeight);
    encodeBool(_allowUpscaling);
    encodeBool(_useMaximumSize);

    encodeInteger(_anamorphicMode);
    encodeInt(_parNum);
    encodeInt(_parDen);
    encodeInt(_width);
    encodeInt(_height);

    encodeInteger(_padMode);
    encodeInt(_padTop);
    encodeInt(_padBottom);
    encodeInt(_padLeft);
    encodeInt(_padRight);
    encodeInteger(_padColorMode);
    encodeObject(_padColorCustom);

    encodeBool(_keepAspectRatio);
    encodeInt(_storageWidth);
    encodeInt(_storageHeight);
    encodeInt(_displayWidth);
    encodeInt(_displayHeight);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_sourceWidth); if (_sourceWidth < 0) { goto fail; }
    decodeInt(_sourceHeight); if (_sourceHeight < 0) { goto fail; }
    decodeInt(_sourceParNum); if (_sourceParNum < 0) { goto fail; }
    decodeInt(_sourceParDen); if (_sourceParDen < 0) { goto fail; }

    decodeInt(_autoCropTop); if (_autoCropTop < 0 || _autoCropTop > _sourceHeight) { goto fail; }
    decodeInt(_autoCropBottom); if (_autoCropBottom < 0 || _autoCropBottom > _sourceHeight) { goto fail; }
    decodeInt(_autoCropLeft); if (_autoCropLeft < 0 || _autoCropLeft > _sourceWidth) { goto fail; }
    decodeInt(_autoCropRight); if (_autoCropRight < 0 || _autoCropLeft > _sourceWidth) { goto fail; }

    decodeInt(_looseAutoCropTop); if (_looseAutoCropTop < 0 || _looseAutoCropTop > _sourceHeight) { goto fail; }
    decodeInt(_looseAutoCropBottom); if (_looseAutoCropBottom < 0 || _looseAutoCropBottom > _sourceHeight) { goto fail; }
    decodeInt(_looseAutoCropLeft); if (_looseAutoCropLeft < 0 || _looseAutoCropLeft > _sourceWidth) { goto fail; }
    decodeInt(_looseAutoCropRight); if (_looseAutoCropRight < 0 || _looseAutoCropRight > _sourceWidth) { goto fail; }

    decodeInt(_angle); if (_angle != 0 && _angle != 90 && _angle != 180 && _angle != 270) { goto fail; }
    decodeBool(_flip);

    decodeInteger(_cropMode);
    if (_cropMode < HBPictureCropModeNone || _cropMode > HBPictureCropModeCustom)
    {
        goto fail;
    }
    decodeInt(_cropTop); if (_cropTop < 0 || _cropTop > _sourceHeight) { goto fail; }
    decodeInt(_cropBottom); if (_cropBottom < 0 || _cropBottom > _sourceHeight) { goto fail; }
    decodeInt(_cropLeft); if (_cropLeft < 0 || _cropLeft > _sourceWidth) { goto fail; }
    decodeInt(_cropRight); if (_cropRight < 0 || _cropRight > _sourceWidth) { goto fail; }

    decodeInteger(_resolutionLimitMode);
    if (_resolutionLimitMode < HBPictureResolutionLimitModeNone || _resolutionLimitMode > HBPictureResolutionLimitModeCustom)
    {
        goto fail;
    }
    decodeInt(_maxWidth); if (_maxWidth < 0 || _maxWidth > HB_MAX_WIDTH) { goto fail; }
    decodeInt(_maxHeight); if (_maxHeight < 0 || _maxHeight > HB_MAX_HEIGHT) { goto fail; }
    decodeBool(_allowUpscaling);
    decodeBool(_useMaximumSize);

    decodeInteger(_anamorphicMode);
    if (_anamorphicMode < HBPictureAnarmophicModeNone || _anamorphicMode > HBPictureAnarmophicModeAuto)
    {
        goto fail;
    }

    decodeInt(_parNum); if (_parNum < 0) { goto fail; }
    decodeInt(_parDen); if (_parDen < 0) { goto fail; }
    decodeInt(_width); if (_width < 0 || _maxWidth > HB_MAX_WIDTH) { goto fail; }
    decodeInt(_height); if (_height < 0 || _maxHeight > HB_MAX_HEIGHT) { goto fail; }

    decodeInteger(_padMode);
    if (_padMode < HBPicturePadModeNone || _padMode > HBPicturePadModeCustom)
    {
        goto fail;
    }
    decodeInt(_padTop); if (_padTop < 0 || _padTop > HB_MAX_HEIGHT) { goto fail; }
    decodeInt(_padBottom); if (_padBottom < 0 || _padBottom > HB_MAX_HEIGHT) { goto fail; }
    decodeInt(_padLeft); if (_padLeft < 0 || _padLeft > HB_MAX_WIDTH) { goto fail; }
    decodeInt(_padRight); if (_padRight < 0 || _padRight > HB_MAX_WIDTH) { goto fail; }
    decodeInteger(_padColorMode);
    if (_padColorMode < HBPicturePadColorModeBlack || _padColorMode > HBPicturePadColorModeCustom)
    {
        goto fail;
    }
    decodeObject(_padColorCustom, NSString);

    decodeInt(_storageWidth); if (_storageWidth < 0) { goto fail; }
    decodeInt(_storageHeight); if (_storageHeight < 0) { goto fail; }

    decodeBool(_keepAspectRatio);
    decodeInt(_displayWidth); if (_displayWidth < 0) { goto fail; }
    decodeInt(_displayHeight); if (_displayHeight < 0) { goto fail; }

    _notificationsEnabled = YES;

    return self;

fail:
    return nil;
}

#pragma mark - Presets

- (void)writeToPreset:(HBMutablePreset *)preset
{
    preset[@"PictureRotate"] = [NSString stringWithFormat:@"angle=%d:hflip=%d", self.angle, self.flip];

    preset[@"PictureWidth"] = @(self.maxWidth);
    preset[@"PictureHeight"] = @(self.maxHeight);

    preset[@"PictureAllowUpscaling"] = @(self.allowUpscaling);
    preset[@"PictureUseMaximumSize"] = @(self.useMaximumSize);

    preset[@"PictureKeepRatio"] = @(self.keepAspectRatio);

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
            preset[@"PicturePAR"] = @"auto";
            break;
    }

    // PAR
    preset[@"PicturePARWidth"] = @(self.parNum);
    preset[@"PicturePARHeight"] = @(self.parDen);

    // Display width
    preset[@"PictureDARWidth"] = @(self.displayWidth);

    // Set crop settings
    switch (self.cropMode)
    {
        case HBPictureCropModeNone:
            preset[@"PictureCropMode"] = @2;
            break;
        case HBPictureCropModeConservative:
            preset[@"PictureCropMode"] = @1;
            break;
        case HBPictureCropModeAutomatic:
            preset[@"PictureCropMode"] = @0;
            break;
        case HBPictureCropModeCustom:
            preset[@"PictureCropMode"] = @3;
            break;
    }

    preset[@"PictureTopCrop"]    = @(self.cropTop);
    preset[@"PictureBottomCrop"] = @(self.cropBottom);
    preset[@"PictureLeftCrop"]   = @(self.cropLeft);
    preset[@"PictureRightCrop"]  = @(self.cropRight);

    // Pad
    preset[@"PicturePadMode"] = @(padModeToString(self.padMode));
    if (self.padMode == HBPicturePadModeCustom)
    {
        preset[@"PicturePadTop"]    = @(self.padTop);
        preset[@"PicturePadBottom"] = @(self.padBottom);
        preset[@"PicturePadLeft"]   = @(self.padLeft);
        preset[@"PicturePadRight"]  = @(self.padRight);
    }
    else
    {
        preset[@"PicturePadTop"]    = @0;
        preset[@"PicturePadBottom"] = @0;
        preset[@"PicturePadLeft"]   = @0;
        preset[@"PicturePadRight"]  = @0;
    }

    switch (self.padColorMode)
    {
        case HBPicturePadColorModeBlack:
            preset[@"PicturePadColor"] = @"black";
            break;
        case HBPicturePadColorModeDarkGray:
            preset[@"PicturePadColor"] = @"darkslategray";
            break;
        case HBPicturePadColorModeGray:
            preset[@"PicturePadColor"] = @"slategray";
            break;
        case HBPicturePadColorModeWhite:
            preset[@"PicturePadColor"] = @"white";
            break;
        case HBPicturePadColorModeCustom:
            preset[@"PicturePadColor"] = self.padColorCustom;
            break;
    }
}

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    NSDictionary<NSString *, NSNumber *> *par = settings[@"PAR"];
    NSDictionary<NSString *, id> *filterList = settings[@"Filters"][@"FilterList"];
    NSDictionary<NSString *, NSNumber *> *cropScale = nil;
    NSDictionary<NSString *, id> *pad = nil;
    NSDictionary<NSString *, id> *rotate = nil;

    for (NSDictionary *dict in filterList)
    {
        if ([dict[@"ID"] intValue] == HB_FILTER_CROP_SCALE)
        {
            cropScale = dict[@"Settings"];
        }
        if ([dict[@"ID"] intValue] == HB_FILTER_PAD)
        {
            pad = dict[@"Settings"];
        }
        if ([dict[@"ID"] intValue] == HB_FILTER_ROTATE)
        {
            rotate = dict[@"Settings"];
        }
    }

    // Dimensions are taken as they are
    // from the hb_job dict

    self.updating = YES;
    self.notificationsEnabled = NO;

    // Rotate
    self.angle = [rotate[@"angle"] intValue];
    self.flip = [rotate[@"hflip"] boolValue];

    // Crop
    int ct = [cropScale[@"crop-top"] intValue];
    int cb = [cropScale[@"crop-bottom"] intValue];
    int cl = [cropScale[@"crop-left"] intValue];
    int cr = [cropScale[@"crop-right"] intValue];

    int cropMode = [preset[@"PictureCropMode"] intValue];
    switch (cropMode)
    {
        case 0:
            self.cropMode = HBPictureCropModeAutomatic;
            break;
        case 1:
            self.cropMode = HBPictureCropModeConservative;
            break;
        case 2:
            self.cropMode = HBPictureCropModeNone;
            break;
        case 3:
            self.cropMode = HBPictureCropModeCustom;
            break;
        default:
            self.cropMode = HBPictureCropModeAutomatic;
            break;
    }

    self.cropTop    = ct;
    self.cropBottom = cb;
    self.cropLeft   = cl;
    self.cropRight  = cr;

    // Resolution
    self.maxWidth = [preset[@"PictureWidth"] intValue];
    self.maxHeight = [preset[@"PictureHeight"] intValue];

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

    if ([preset[@"PicturePAR"] isEqualToString:@"auto"])
    {
        self.anamorphicMode = HBPictureAnarmophicModeAuto;
    }
    else if ([preset[@"PicturePAR"] isEqualToString:@"custom"])
    {
        self.anamorphicMode = HBPictureAnarmophicModeCustom;
    }
    else
    {
        self.anamorphicMode = HBPictureAnarmophicModeNone;
    }

    self.parNum = [par[@"Num"] intValue];
    self.parDen = [par[@"Den"] intValue];

    self.width = [cropScale[@"width"] intValue];
    self.height = [cropScale[@"height"] intValue];

    self.keepAspectRatio = [preset[@"PictureKeepRatio"] boolValue];
    self.allowUpscaling = [preset[@"PictureAllowUpscaling"] boolValue];
    self.useMaximumSize = [preset[@"PictureUseMaximumSize"] boolValue];

    // Pad
    NSString *padMode = preset[@"PicturePadMode"];
    if ([padMode isEqualToString:@"fill"]) { self.padMode = HBPicturePadModeFill; }
    else if ([padMode isEqualToString:@"letterbox"]) { self.padMode = HBPicturePadModeFillHeight; }
    else if ([padMode isEqualToString:@"pillarbox"]) { self.padMode = HBPicturePadModeFillWidth; }
    else if ([padMode isEqualToString:@"custom"]) { self.padMode = HBPicturePadModeCustom; }
    else { self.padMode = HBPicturePadModeNone; }

    NSString *padColor = preset[@"PicturePadColor"];
    if ([padColor isEqualToString:@"black"]) { self.padColorMode = HBPicturePadColorModeBlack; }
    else if ([padColor isEqualToString:@"darkslategray"]) { self.padColorMode = HBPicturePadColorModeDarkGray; }
    else if ([padColor isEqualToString:@"slategray"]) { self.padColorMode = HBPicturePadColorModeGray; }
    else if ([padColor isEqualToString:@"white"]) { self.padColorMode = HBPicturePadColorModeWhite; }
    else if (padColor.length) { self.padColorMode = HBPicturePadColorModeCustom; self.padColorCustom = padColor; }
    else { self.padColorMode = HBPicturePadColorModeBlack; self.padColorCustom = @""; }

    self.padTop    = [pad[@"top"] intValue];
    self.padBottom = [pad[@"bottom"] intValue];
    self.padLeft   = [pad[@"left"] intValue];
    self.padRight  = [pad[@"right"] intValue];

    // Final dimensions
    self.storageWidth = self.width + self.padLeft + self.padRight;
    self.storageHeight = self.height + self.padTop + self.padBottom;

    self.displayWidth = ((double)self.parNum / self.parDen) * self.storageWidth + 0.5;
    self.displayHeight = self.storageHeight;

    self.updating = NO;
    self.notificationsEnabled = YES;

    [self postChangedNotification];
}

@end
