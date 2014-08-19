/*  HBFilters.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

/**
 *  Filters settings.
 */
@interface HBFilters : NSObject

- (void)prepareFiltersForPreset:(NSMutableDictionary *)preset;
- (void)applySettingsFromPreset:(NSDictionary *)preset;

@property (nonatomic, readwrite) NSInteger detelecine;
@property (nonatomic, readwrite, copy) NSString *detelecineCustomString;

@property (nonatomic, readwrite) NSInteger deinterlace;
@property (nonatomic, readwrite, copy) NSString *deinterlaceCustomString;

@property (nonatomic, readwrite) NSInteger decomb;
@property (nonatomic, readwrite, copy) NSString *decombCustomString;

@property (nonatomic, readwrite, copy) NSString *denoise;
@property (nonatomic, readwrite, copy) NSString *denoisePreset;
@property (nonatomic, readwrite, copy) NSString *denoiseTune;
@property (nonatomic, readwrite, copy) NSString *denoiseCustomString;

@property (nonatomic, readwrite) NSInteger deblock;
@property (nonatomic, readwrite) BOOL grayscale;

@property (nonatomic, readwrite) BOOL useDecomb;

/**
 *  A textual summary of the filters settings.
 */
@property (nonatomic, readonly) NSString *summary;

/**
 *  Getters to get the possible values for the filters.
 */
+ (NSDictionary *)denoisePresetDict;
+ (NSDictionary *)nlmeansTunesDict;
+ (NSDictionary *)denoiseTypesDict;

@property (nonatomic, readonly) NSArray *detelecineSettings;
@property (nonatomic, readonly) NSArray *deinterlaceSettings;
@property (nonatomic, readonly) NSArray *decombSettings;

@property (nonatomic, readonly) NSArray *denoiseTypes;
@property (nonatomic, readonly) NSArray *denoisePresets;
@property (nonatomic, readonly) NSArray *denoiseTunes;

@end

/**
 *  A collection of NSValueTransformer to map between localized UI values
 *  and the internals values.
 */

@interface HBGenericDictionaryTransformer : NSValueTransformer
@property (nonatomic, retain) NSDictionary *dict;
@end

@interface HBDenoisePresetTransformer : HBGenericDictionaryTransformer
@end

@interface HBDenoiseTuneTransformer : HBGenericDictionaryTransformer
@end

@interface HBDenoiseTransformer : HBGenericDictionaryTransformer
@end