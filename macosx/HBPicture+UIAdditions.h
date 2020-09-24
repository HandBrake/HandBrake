/*  HBPicture+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBPicture.h>

@interface HBPicture (UIAdditions)

/**
 *  UI enabled bindings
 */
@property (nonatomic, readonly) NSString *info;
@property (nonatomic, readonly) NSString *shortInfo;
@property (nonatomic, readonly) NSString *summary;

@property (nonatomic, readonly, getter=isCustomResolutionLimitEnabled) BOOL customResolutionLimitEnabled;

@property (nonatomic, readonly) int maxTopCrop;
@property (nonatomic, readonly) int maxBottomCrop;
@property (nonatomic, readonly) int maxLeftCrop;
@property (nonatomic, readonly) int maxRightCrop;

@property (nonatomic, readonly) int maxTopPadding;
@property (nonatomic, readonly) int maxBottomPadding;
@property (nonatomic, readonly) int maxLeftPadding;
@property (nonatomic, readonly) int maxRightPadding;

@property (nonatomic, readonly, getter=isCustomPaddingEnabled) BOOL customPaddingEnabled;
@property (nonatomic, readonly, getter=isCustomPaddingColorEnabled) BOOL customPaddingColorEnabled;

@property (nonatomic, readonly, getter=isKeepDisplayAspectEditable) BOOL keepDisplayAspectEditable;
@property (nonatomic, readonly, getter=isCustomAnamorphicEnabled) BOOL customAnamorphicEnabled;

@end
