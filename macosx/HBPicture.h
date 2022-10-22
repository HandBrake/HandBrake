/*  HBPicture.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBPresetCoding.h>

@class HBTitle;

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, HBPictureCropMode) {
    HBPictureCropModeNone,
    HBPictureCropModeConservative,
    HBPictureCropModeAutomatic,
    HBPictureCropModeCustom
};

typedef NS_ENUM(NSUInteger, HBPictureResolutionLimitMode) {
    HBPictureResolutionLimitModeNone,
    HBPictureResolutionLimitMode8K,
    HBPictureResolutionLimitMode4K,
    HBPictureResolutionLimitMode1440p,
    HBPictureResolutionLimitMode1080p,
    HBPictureResolutionLimitMode720p,
    HBPictureResolutionLimitMode576p,
    HBPictureResolutionLimitMode480p,
    HBPictureResolutionLimitModeCustom,
};

typedef NS_ENUM(NSUInteger, HBPictureAnarmophicMode) {
    HBPictureAnarmophicModeNone,
    HBPictureAnarmophicModeStrict,
    HBPictureAnarmophicModeLoose,
    HBPictureAnarmophicModeCustom,
    HBPictureAnarmophicModeAuto
};

typedef NS_ENUM(NSUInteger, HBPicturePadMode) {
    HBPicturePadModeNone,
    HBPicturePadModeFill,
    HBPicturePadModeFillHeight,
    HBPicturePadModeFillWidth,
    HBPicturePadModeCustom
};

typedef NS_ENUM(NSUInteger, HBPicturePadColorMode) {
    HBPicturePadColorModeBlack,
    HBPicturePadColorModeDarkGray,
    HBPicturePadColorModeGray,
    HBPicturePadColorModeWhite,
    HBPicturePadColorModeCustom,
};

extern NSString * const HBPictureChangedNotification;

/**
 * HBPicture
 */
@interface HBPicture : NSObject <NSSecureCoding, NSCopying>

/**
 *  Source
 */
@property (nonatomic, readonly) int sourceWidth;
@property (nonatomic, readonly) int sourceHeight;
@property (nonatomic, readonly) int sourceParNum;
@property (nonatomic, readonly) int sourceParDen;
@property (nonatomic, readonly) int sourceDisplayWidth;

/**
 *  Rotate
 */
@property (nonatomic, readwrite) int angle;
@property (nonatomic, readwrite) BOOL flip;

/**
 *  Crop
 */
@property (nonatomic, readwrite) HBPictureCropMode cropMode;
@property (nonatomic, readwrite) int cropTop;
@property (nonatomic, readwrite) int cropBottom;
@property (nonatomic, readwrite) int cropLeft;
@property (nonatomic, readwrite) int cropRight;

/**
 *  Size
 */
@property (nonatomic, readwrite) HBPictureResolutionLimitMode resolutionLimitMode;
@property (nonatomic, readwrite) int maxWidth;
@property (nonatomic, readwrite) int maxHeight;
@property (nonatomic, readwrite) BOOL allowUpscaling;
@property (nonatomic, readwrite) BOOL useMaximumSize;

@property (nonatomic, readwrite) HBPictureAnarmophicMode anamorphicMode;

@property (nonatomic, readwrite) int parNum;
@property (nonatomic, readwrite) int parDen;
@property (nonatomic, readwrite) int width;
@property (nonatomic, readwrite) int height;

/**
 *  Pad
 */
@property (nonatomic, readwrite) HBPicturePadMode padMode;
@property (nonatomic, readwrite) int padTop;
@property (nonatomic, readwrite) int padBottom;
@property (nonatomic, readwrite) int padLeft;
@property (nonatomic, readwrite) int padRight;
@property (nonatomic, readwrite) HBPicturePadColorMode padColorMode;
@property (nonatomic, readwrite) NSString *padColorCustom;

/**
 * Output sizes
 */
@property (nonatomic, readonly) int storageWidth;
@property (nonatomic, readonly) int storageHeight;

@property (nonatomic, readwrite) BOOL keepAspectRatio;
@property (nonatomic, readwrite) int displayWidth;
@property (nonatomic, readonly) int displayHeight;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
