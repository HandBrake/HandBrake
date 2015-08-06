/*  HBPicture+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBPicture.h"

@interface HBPicture (UIAdditions)

/**
 *  UI enabled bindings
 */
@property (nonatomic, readonly) NSString *info;
@property (nonatomic, readonly) NSString *sourceInfo;
@property (nonatomic, readonly) NSString *summary;

@property (nonatomic, readonly) int maxWidth;
@property (nonatomic, readonly) int maxHeight;

@property (nonatomic, readonly) int maxVerticalCrop;
@property (nonatomic, readonly) int maxHorizontalCrop;

@property (nonatomic, readonly, getter=isWidthEditable) BOOL widthEditable;
@property (nonatomic, readonly, getter=isHeightEditable) BOOL heightEditable;

@property (nonatomic, readonly, getter=isKeepDisplayAspect) BOOL keepDisplayAspectEditable;
@property (nonatomic, readonly, getter=isCustomAnamorphicEnabled) BOOL customAnamorphicEnabled;

@end
