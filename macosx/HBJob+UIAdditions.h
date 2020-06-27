/*  HBJob+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <HandBrakeKit/HBJob.h>

@interface HBJob (UIAdditions)

@property (nonatomic, readonly) BOOL mp4OptionsEnabled;
@property (nonatomic, readonly) BOOL mp4iPodCompatibleEnabled;

@property (nonatomic, readonly) NSArray<NSString *> *angles;
@property (nonatomic, readonly) NSArray<NSString *> *containers;

@property (nonatomic, readonly) NSAttributedString *attributedDescription;

@property (nonatomic, readonly) NSString *shortDescription;
@property (nonatomic, readonly) NSString *filtersShortDescription;

@end

@interface HBContainerTransformer : NSValueTransformer
@end

@interface HBURLTransformer : NSValueTransformer
@end
