/*  HBPicture.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBTitle;

extern NSString * const HBPictureChangedNotification;

/**
 * HBPicture
 */
@interface HBPicture : NSObject <NSCoding>

- (instancetype)initWithTitle:(HBTitle *)title;

- (void)applyPictureSettingsFromQueue:(NSDictionary *)queueToApply;
- (void)preparePictureForQueueFileJob:(NSMutableDictionary *)queueFileJob;

- (void)preparePictureForPreset:(NSMutableDictionary *)preset;
- (void)applySettingsFromPreset:(NSDictionary *)preset;

@property (nonatomic, readwrite) int width;
@property (nonatomic, readwrite) int height;

@property (nonatomic, readwrite) int keepDisplayAspect;
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
 *  UI enabled bindings
 */
@property (nonatomic, readonly) NSString *info;

@property (nonatomic, readonly) int maxWidth;
@property (nonatomic, readonly) int maxHeight;

@property (nonatomic, readonly) int maxVerticalCrop;
@property (nonatomic, readonly) int maxHorizontalCrop;

@property (nonatomic, readonly, getter=isWidthEditable) BOOL widthEditable;
@property (nonatomic, readonly, getter=isHeightEditable) BOOL heightEditable;

@property (nonatomic, readonly, getter=isKeepDisplayAspect) BOOL keepDisplayAspectEditable;
@property (nonatomic, readonly, getter=isCustomAnamorphicEnabled) BOOL customAnamorphicEnabled;

@property (nonatomic, readwrite, assign) HBTitle *title;


@end
