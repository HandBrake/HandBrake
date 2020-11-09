/*  HBPicture.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBPresetCoding.h>

@class HBTitle;

NS_ASSUME_NONNULL_BEGIN

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

typedef NS_ENUM(NSUInteger, HBPicturePaddingMode) {
    HBPicturePaddingModeNone,
    HBPicturePaddingModeFill,
    HBPicturePaddingModeFillHeight,
    HBPicturePaddingModeFillWidth,
    HBPicturePaddingModeCustom
};

typedef NS_ENUM(NSUInteger, HBPicturePaddingColorMode) {
    HBPicturePaddingColorModeBlack,
    HBPicturePaddingColorModeWhite,
    HBPicturePaddingColorModeCustom,
};

extern NSString * const HBPictureChangedNotification;

/**
 * HBPicture
 */
@interface HBPicture : NSObject <NSSecureCoding, NSCopying>

/**
 *  Rotation
 */
@property (nonatomic, readwrite) int rotate;
@property (nonatomic, readwrite) BOOL flip;

/**
 *  Size
 */
@property (nonatomic, readwrite) HBPictureResolutionLimitMode resolutionLimitMode;
@property (nonatomic, readwrite) int maxWidth;
@property (nonatomic, readwrite) int maxHeight;
@property (nonatomic, readwrite) BOOL allowUpscaling;
@property (nonatomic, readwrite) BOOL useMaximumSize;

@property (nonatomic, readwrite) int width;
@property (nonatomic, readwrite) int height;

@property (nonatomic, readwrite) BOOL keepDisplayAspect;
@property (nonatomic, readwrite) HBPictureAnarmophicMode anamorphicMode;

/**
 *  Custom anamorphic settings
 */
@property (nonatomic, readwrite) int displayWidth;
@property (nonatomic, readwrite) int parWidth;
@property (nonatomic, readwrite) int parHeight;

/**
 *  Crop settings
 */
@property (nonatomic, readwrite) BOOL autocrop;
@property (nonatomic, readwrite) int cropTop;
@property (nonatomic, readwrite) int cropBottom;
@property (nonatomic, readwrite) int cropLeft;
@property (nonatomic, readwrite) int cropRight;

/**
 *  Padding settings
 */
@property (nonatomic, readwrite) HBPicturePaddingMode paddingMode;
@property (nonatomic, readwrite) int paddingTop;
@property (nonatomic, readwrite) int paddingBottom;
@property (nonatomic, readwrite) int paddingLeft;
@property (nonatomic, readwrite) int paddingRight;
@property (nonatomic, readwrite) HBPicturePaddingColorMode paddingColorMode;
@property (nonatomic, readwrite) NSString *paddingColorCustom;

/**
 *  Source size
 */
@property (nonatomic, readonly) int sourceWidth;
@property (nonatomic, readonly) int sourceHeight;
@property (nonatomic, readonly) int sourceDisplayWidth;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
