/*  HBFilters+UIAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBFilters.h>

@interface HBFilters (UIAdditions)

/**
 *  Getters to get the possible values for the filters.
 */
+ (NSDictionary *)detelecinePresetsDict;

+ (NSDictionary *)combDetectionPresetsDict;

+ (NSDictionary *)deinterlaceTypesDict;
+ (NSDictionary *)decombPresetsDict;
+ (NSDictionary *)yadifPresetsDict;
+ (NSDictionary *)bwdifPresetsDict;

+ (NSDictionary *)denoisePresetDict;
+ (NSDictionary *)nlmeansTunesDict;
+ (NSDictionary *)denoiseTypesDict;

+ (NSDictionary *)chromaSmoothPresetDict;
+ (NSDictionary *)chromaSmoothTunesDict;

+ (NSDictionary *)sharpenPresetDict;
+ (NSDictionary *)sharpenTunesDict;
+ (NSDictionary *)sharpenTypesDict;

+ (NSDictionary *)deblockPresetDict;
+ (NSDictionary *)deblockTunesDict;

+ (NSDictionary *)colorspacePresetDict;

@property (nonatomic, readonly) BOOL customDetelecineSelected;

@property (nonatomic, readonly) BOOL customCombDetectionSelected;

@property (nonatomic, readonly) BOOL deinterlaceEnabled;
@property (nonatomic, readonly) BOOL customDeinterlaceSelected;

@property (nonatomic, readonly) BOOL chromaSmoothEnabled;
@property (nonatomic, readonly) BOOL customChromaSmoothSelected;

@property (nonatomic, readonly) BOOL sharpenEnabled;
@property (nonatomic, readonly) BOOL customSharpenSelected;
@property (nonatomic, readonly) BOOL sharpenTunesAvailable;

@property (nonatomic, readonly) BOOL deblockTunesAvailable;
@property (nonatomic, readonly) BOOL customDeblockSelected;

@property (nonatomic, readonly) BOOL customColorspaceSelected;

@property (nonatomic, readonly) NSArray<NSString *> *detelecineSettings;

@property (nonatomic, readonly) NSArray<NSString *> *combDetectionSettings;

@property (nonatomic, readonly) NSArray<NSString *> *deinterlaceTypes;
@property (nonatomic, readonly) NSArray<NSString *> *deinterlacePresets;

@property (nonatomic, readonly) NSArray<NSString *> *denoiseTypes;
@property (nonatomic, readonly) NSArray<NSString *> *denoisePresets;
@property (nonatomic, readonly) NSArray<NSString *> *denoiseTunes;

@property (nonatomic, readonly) NSArray<NSString *> *chromaSmoothPresets;
@property (nonatomic, readonly) NSArray<NSString *> *chromaSmoothTunes;

@property (nonatomic, readonly) NSArray<NSString *> *sharpenTypes;
@property (nonatomic, readonly) NSArray<NSString *> *sharpenPresets;
@property (nonatomic, readonly) NSArray<NSString *> *sharpenTunes;

@property (nonatomic, readonly) NSArray<NSString *> *deblockPresets;
@property (nonatomic, readonly) NSArray<NSString *> *deblockTunes;

@property (nonatomic, readonly) NSArray<NSString *> *colorspacePresets;

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

@interface HBChromaSmoothTransformer : HBGenericDictionaryTransformer
@end

@interface HBChromaSmoothTuneTransformer : HBGenericDictionaryTransformer
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

@interface HBColorspaceTransformer : HBGenericDictionaryTransformer
@end
