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

@property (nonatomic, readwrite, assign) HBTitle *title;

@end

