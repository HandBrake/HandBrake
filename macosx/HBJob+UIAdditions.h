/*  HBJob+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBJob.h"

@interface HBJob (UIAdditions)

@property (nonatomic, readonly) BOOL mp4OptionsEnabled;
@property (nonatomic, readonly) BOOL mp4iPodCompatibleEnabled;

@property (nonatomic, readonly) NSArray *angles;

@property (nonatomic, readonly) NSAttributedString *attributedDescription;

@end

@interface HBContainerTransformer : NSValueTransformer
@end

@interface HBURLTransformer : NSValueTransformer
@end
