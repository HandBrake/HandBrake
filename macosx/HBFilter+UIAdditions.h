/*  HBFilter+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <HandBrakeKit/HBFilter.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBFilter (UIAdditions)

+ (NSString *)localizedNameForFilterID:(int)filterID;

@property (nonatomic, readonly) NSString *localizedName;
@property (nonatomic, readonly) BOOL customSelected;

@property (nonatomic, readonly) NSArray<NSString *> *presets;
@property (nonatomic, readonly) NSArray<NSString *> *tunes;

@end

// A collection of NSValueTransformer to map between localized UI values
// and the internals values.

@interface HBGenericDictionaryTransformer : NSValueTransformer
@property (nonatomic, strong) NSDictionary *dict;
@end

@interface HBFilterTuneTransformer : HBGenericDictionaryTransformer
- (instancetype)initWithWithFilterID:(int)filterID;
@end

@interface HBFilterPresetTransformer : HBGenericDictionaryTransformer
- (instancetype)initWithWithFilterID:(int)filterID;
@end

NS_ASSUME_NONNULL_END
