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

+ (NSDictionary *)deinterlaceTypesDict;
+ (NSDictionary *)decombPresetsDict;
+ (NSDictionary *)deinterlacePresetsDict;

+ (NSDictionary *)denoisePresetDict;
+ (NSDictionary *)nlmeansTunesDict;
+ (NSDictionary *)denoiseTypesDict;

- (BOOL)customDetelecineSelected;

- (BOOL)deinterlaceEnabled;
- (BOOL)customDeinterlaceSelected;

- (BOOL)denoiseEnabled;
- (BOOL)customDenoiseSelected;
- (BOOL)denoiseTunesAvailable;

- (NSString *)deblockSummary;

@property (nonatomic, readonly) NSArray *detelecineSettings;

@property (nonatomic, readonly) NSArray *deinterlaceTypes;
@property (nonatomic, readonly) NSArray *deinterlacePresets;

@property (nonatomic, readonly) NSArray *denoiseTypes;
@property (nonatomic, readonly) NSArray *denoisePresets;
@property (nonatomic, readonly) NSArray *denoiseTunes;

/**
 *  A textual summary of the filters settings.
 */
@property (nonatomic, readonly) NSString *summary;

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

@interface HBCustomFilterTransformer : NSValueTransformer
@end

