/*  HBFilters+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBFilters.h"

@interface HBFilters (UIAdditions)

/**
 *  Getters to get the possible values for the filters.
 */
+ (NSDictionary *)detelecinePresetsDict;

+ (NSDictionary *)combDetectionPresetsDict;

+ (NSDictionary *)deinterlaceTypesDict;
+ (NSDictionary *)decombPresetsDict;
+ (NSDictionary *)deinterlacePresetsDict;

+ (NSDictionary *)denoisePresetDict;
+ (NSDictionary *)nlmeansTunesDict;
+ (NSDictionary *)denoiseTypesDict;

+ (NSDictionary *)sharpenPresetDict;
+ (NSDictionary *)sharpenTunesDict;
+ (NSDictionary *)sharpenTypesDict;

+ (NSDictionary *)deblockPresetDict;
+ (NSDictionary *)deblockTunesDict;

- (BOOL)customDetelecineSelected;

@property (nonatomic, readonly) BOOL customCombDetectionSelected;

- (BOOL)deinterlaceEnabled;
- (BOOL)customDeinterlaceSelected;

- (BOOL)sharpenEnabled;
- (BOOL)customSharpenSelected;
- (BOOL)sharpenTunesAvailable;

- (BOOL)deblockTunesAvailable;
- (BOOL)customDeblockSelected;

@property (nonatomic, readonly) NSArray *detelecineSettings;

@property (nonatomic, readonly) NSArray *combDetectionSettings;

@property (nonatomic, readonly) NSArray *deinterlaceTypes;
@property (nonatomic, readonly) NSArray *deinterlacePresets;

@property (nonatomic, readonly) NSArray *sharpenTypes;
@property (nonatomic, readonly) NSArray *sharpenPresets;
@property (nonatomic, readonly) NSArray *sharpenTunes;

@property (nonatomic, readonly) NSArray *deblockPresets;
@property (nonatomic, readonly) NSArray *deblockTunes;

@end

/**
 *  A collection of NSValueTransformer to map between localized UI values
 *  and the internals values.
 */

@interface HBGenericDictionaryTransformer : NSValueTransformer
@property (nonatomic, strong) NSDictionary *dict;
@end

@interface HBDetelecineTransformer : HBGenericDictionaryTransformer
@end

@interface HBCombDetectionTransformer : HBGenericDictionaryTransformer
@end

@interface HBDeinterlaceTransformer : HBGenericDictionaryTransformer
@end

@interface HBDeinterlacePresetTransformer : HBGenericDictionaryTransformer
@end

@interface HBDenoisePresetTransformer : HBGenericDictionaryTransformer
@end

@interface HBDenoiseTuneTransformer : HBGenericDictionaryTransformer
@end

@interface HBDenoiseTransformer : HBGenericDictionaryTransformer
@end

@interface HBSharpenPresetTransformer : HBGenericDictionaryTransformer
@end

@interface HBSharpenTuneTransformer : HBGenericDictionaryTransformer
@end

@interface HBSharpenTransformer : HBGenericDictionaryTransformer
@end

@interface HBDeblockTuneTransformer : HBGenericDictionaryTransformer
@end

@interface HBDeblockTransformer : HBGenericDictionaryTransformer
@end
