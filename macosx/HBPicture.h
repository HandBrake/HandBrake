/*  HBPicture.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPresetCoding.h"

@class HBTitle;

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HBPictureChangedNotification;

/**
 * HBPicture
 */
@interface HBPicture : NSObject <NSSecureCoding, NSCopying, HBPresetCoding>

- (instancetype)initWithTitle:(HBTitle *)title;

@property (nonatomic, readwrite) int width;
@property (nonatomic, readwrite) int height;

@property (nonatomic, readwrite) BOOL keepDisplayAspect;
@property (nonatomic, readwrite) int anamorphicMode;
@property (nonatomic, readwrite) int modulus;

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
 *  Source size
 */
@property (nonatomic, readonly) int sourceWidth;
@property (nonatomic, readonly) int sourceHeight;
@property (nonatomic, readonly) int sourceDisplayWidth;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
