/*  HBVideoFilters.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBVideoFilters.h"
#import "HBLocalizationUtilities.h"
#import "HBMutablePreset.h"

#include "handbrake/handbrake.h"

NS_ASSUME_NONNULL_BEGIN

@implementation HBVideoFilters

- (NSArray<NSNumber *> *)availableFilters
{
    return @[
        @(HB_FILTER_DETELECINE),
        @(HB_FILTER_COMB_DETECT),
        @(HB_FILTER_DECOMB),
        @(HB_FILTER_YADIF),
        @(HB_FILTER_BWDIF),
        @(HB_FILTER_DEBLOCK),
        @(HB_FILTER_DEBAND),
        @(HB_FILTER_DENOISE),
        @(HB_FILTER_BM3D),
        @(HB_FILTER_NLMEANS),
        @(HB_FILTER_CHROMA_SMOOTH),
        @(HB_FILTER_LAPSHARP),
        @(HB_FILTER_UNSHARP),
        @(HB_FILTER_GRAYSCALE),
        @(HB_FILTER_COLORSPACE),
    ];
}

- (NSArray<HBFilterGroup *> *)availableFilterGroups
{
    NSMutableArray<HBFilterGroup *> *groups = [NSMutableArray array];

    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_FILTER_DETELECINE]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_FILTER_COMB_DETECT]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilters:@[@(HB_FILTER_DECOMB),
                                                               @(HB_FILTER_YADIF),
                                                               @(HB_FILTER_BWDIF)]
                                                   groupName:HBKitLocalizedString(@"Deinterlace", "HBFilter")]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_FILTER_DEBLOCK]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_FILTER_DEBAND]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilters:@[@(HB_FILTER_DENOISE),
                                                               @(HB_FILTER_BM3D),
                                                               @(HB_FILTER_NLMEANS)]
                                                   groupName:HBKitLocalizedString(@"Denoise", "HBFilter")]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_FILTER_CHROMA_SMOOTH]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilters:@[@(HB_FILTER_LAPSHARP),
                                                               @(HB_FILTER_UNSHARP)]
                                                   groupName:HBKitLocalizedString(@"Sharpen", "HBFilter")]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_FILTER_GRAYSCALE]];
    [groups addObject:[[HBFilterGroup alloc] initWithFilterID:HB_FILTER_COLORSPACE]];

    return groups;
}

- (void)writeToPreset:(HBMutablePreset *)preset
{
    // Turn every filter off
    preset[@"PictureDeinterlaceFilter"]  = @"off";
    preset[@"PictureCombDetectPreset"]   = @"off";
    preset[@"PictureDetelecine"]         = @"off";
    preset[@"PictureDebandPreset"]       = @"off";
    preset[@"PictureDenoisePreset"]      = @"off";
    preset[@"PictureChromaSmoothPreset"] = @"off";
    preset[@"PictureSharpenPreset"]      = @"off";
    preset[@"PictureDeblockPreset"]      = @"off";
    preset[@"VideoGrayScale"]            = @NO;
    preset[@"PictureColorspacePreset"]   = @"off";

    // Write the enabled filter values
    for (HBFilter *filter in self.filters)
    {
        [filter writeToPreset:preset];
    }
}

- (void)addComplexFilterWithKey:(NSString *)key fromPreset:(HBPreset *)filterPreset
{
    NSString *name, *preset, *tune, *custom;

    name   = filterPreset[[key stringByAppendingString:@"Filter"]];
    preset = filterPreset[[key stringByAppendingString:@"Preset"]];
    tune   = filterPreset[[key stringByAppendingString:@"Tune"]];
    custom = filterPreset[[key stringByAppendingString:@"Custom"]];

    if ([name isKindOfClass:[NSString class]] &&
        [name isEqualToString:@"off"] == NO)
    {
        HBFilter *filter = [[HBFilter alloc] initWithFilter:name
                                                     preset:preset
                                                       tune:tune
                                                     custom:custom];
        filter.delegate = self;
        filter.undo = self.undo;

        [self insertObject:filter inFiltersAtIndex:self.filters.count];
    }
}

- (void)addFilterWithName:(NSString *)name key:(NSString *)key fromPreset:(HBPreset *)filterPreset
{
    NSString *preset, *tune, *custom;

    preset = filterPreset[[key stringByAppendingString:@"Preset"]];
    tune   = filterPreset[[key stringByAppendingString:@"Tune"]];
    custom = filterPreset[[key stringByAppendingString:@"Custom"]];

    if ([preset isKindOfClass:[NSString class]] &&
        [preset isEqualToString:@"off"] == NO)
    {
        HBFilter *filter = [[HBFilter alloc] initWithFilter:name
                                                     preset:preset
                                                       tune:tune
                                                     custom:custom];
        filter.delegate = self;
        filter.undo = self.undo;

        [self insertObject:filter inFiltersAtIndex:self.filters.count];
    }
}

- (void)addSimpleFilterWithName:(NSString *)name key:(NSString *)key fromPreset:(HBPreset *)filterPreset
{
    NSString *tune, *custom;
    id preset;

    preset = filterPreset[key];
    tune   = filterPreset[[key stringByAppendingString:@"Tune"]];
    custom = filterPreset[[key stringByAppendingString:@"Custom"]];

    if ([preset isKindOfClass:[NSNumber class]] && [preset boolValue])
    {
        preset = @"default";
    }

    if ([preset isKindOfClass:[NSString class]] &&
        [preset isEqualToString:@"off"] == NO)
    {
        HBFilter *filter = [[HBFilter alloc] initWithFilter:name
                                                     preset:preset
                                                       tune:tune
                                                     custom:custom];
        filter.delegate = self;
        filter.undo = self.undo;

        [self insertObject:filter inFiltersAtIndex:self.filters.count];
    }
}

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    self.notificationsEnabled = NO;

    [self removeFiltersAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, self.filters.count)]];

    if ([preset[@"UsesPictureFilters"] boolValue])
    {
        [self addSimpleFilterWithName:@"detelecine" key:@"PictureDetelecine" fromPreset:preset];
        [self addFilterWithName:@"combdetect" key:@"PictureCombDetect" fromPreset:preset];
        [self addComplexFilterWithKey:@"PictureDeinterlace" fromPreset:preset];
        [self addFilterWithName:@"deband" key:@"PictureDeband" fromPreset:preset];
        [self addFilterWithName:@"deblock" key:@"PictureDeblock" fromPreset:preset];
        [self addComplexFilterWithKey:@"PictureDenoise" fromPreset:preset];
        [self addFilterWithName:@"chromasmooth" key:@"PictureChromaSmooth" fromPreset:preset];
        [self addComplexFilterWithKey:@"PictureSharpen" fromPreset:preset];
        [self addSimpleFilterWithName:@"grayscale" key:@"VideoGrayScale" fromPreset:preset];
        [self addFilterWithName:@"colorspace" key:@"PictureColorspace" fromPreset:preset];
    }

    self.notificationsEnabled = YES;
    [self postChangedNotification];
}

@end

NS_ASSUME_NONNULL_END
