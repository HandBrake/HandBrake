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

@property (nonatomic, readonly, getter=isCustomCropEnabled) BOOL customCropEnabled;

@property (nonatomic, readonly, getter=isCustomResolutionLimitEnabled) BOOL customResolutionLimitEnabled;

@property (nonatomic, readonly, getter=isCustomPadEnabled) BOOL customPadEnabled;
@property (nonatomic, readonly, getter=isCustomPadColorEnabled) BOOL customPadColorEnabled;

@property (nonatomic, readonly, getter=isCustomAnamorphicEnabled) BOOL customAnamorphicEnabled;

@end
