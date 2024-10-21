/*  HBPreset.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreset.h"
#import "HBMutablePreset.h"

#include "handbrake/preset.h"

#import "NSJSONSerialization+HBAdditions.h"
#import "HBLocalizationUtilities.h"

@interface HBPreset ()

///  The actual content of the preset.
@property (nonatomic, strong, nullable) NSMutableDictionary *content;

@end

@implementation HBPreset

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _name = @"New Preset";
        _presetDescription = @"";
        _content = [[NSMutableDictionary alloc] init];
        self.isLeaf = YES;
    }
    return self;
}

- (instancetype)initWithPreset:(HBPreset *)preset
{
    self = [super init];
    if (self)
    {
        _name = preset.name;
        _presetDescription = preset.presetDescription;
        _content = [preset.content mutableCopy];
        self.isLeaf = preset.isLeaf;

        for (HBPreset *child in preset.children)
        {
            HBPreset *mutableChild = [[[self class] alloc] initWithPreset:child];
            [self insertObject:mutableChild inChildrenAtIndex:0];
        }
    }
    return self;
}

- (instancetype)initWithName:(NSString *)title content:(NSDictionary *)content builtIn:(BOOL)builtIn
{
    self = [self init];
    if (self)
    {
        _name = [title copy];
        _isBuiltIn = builtIn;
        _content = [content mutableCopy];
        if ([content[@"PresetDescription"] isKindOfClass:[NSString class]])
        {
            _presetDescription = [content[@"PresetDescription"] copy];
        }
    }
    return self;
}

- (instancetype)initWithCategoryName:(NSString *)title builtIn:(BOOL)builtIn
{
    self = [self init];
    if (self)
    {
        _name = [title copy];
        _isBuiltIn = builtIn;
        self.isLeaf = NO;
    }
    return self;
}

- (instancetype)initWithDictionary:(NSDictionary *)dict
{
    NSParameterAssert(dict);

    NSString *name = dict[@"PresetName"] ? dict[@"PresetName"] : @"Unnamed preset";
    BOOL builtIn = [dict[@"Type"] boolValue] ? NO : YES;
    BOOL defaultPreset = [dict[@"Default"] boolValue];

    if ([dict[@"Folder"] boolValue])
    {
        self = [self initWithCategoryName:name builtIn:builtIn];

        for (NSDictionary *childDict in [dict[@"ChildrenArray"] reverseObjectEnumerator])
        {
            HBPreset *childPreset = [[HBPreset alloc] initWithDictionary:childDict];
            [self insertObject:childPreset inChildrenAtIndex:0];
        }
    }
    else
    {
        self = [self initWithName:name
                          content:dict
                          builtIn:builtIn];
        self.isDefault = defaultPreset;
    }

    return self;
}

- (nullable instancetype)initWithContentsOfURL:(NSURL *)url error:(NSError * __autoreleasing *)outError
{
    NSParameterAssert(url);

    NSArray *presetsArray;
    NSString *presetsString;

    if ([url.pathExtension isEqualToString:@"json"])
    {
        NSData *data = [[NSData alloc] initWithContentsOfURL:url];
        presetsString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    }
    else
    {
        NSArray *array = [[NSArray alloc] initWithContentsOfURL:url];
        if ([NSJSONSerialization isValidJSONObject:array])
        {
            presetsString = [NSJSONSerialization HB_StringWithJSONObject:array options:0 error:NULL];
        }
    }

    // Run the json through the libhb import function
    // to avoid importing unknowns keys.
    if (presetsString.length)
    {
        char *importedJson;
        hb_presets_import_json(presetsString.UTF8String, &importedJson);

        if (importedJson)
        {
            id importedPresets = [NSJSONSerialization HB_JSONObjectWithUTF8String:importedJson options:0 error:NULL];
            free(importedJson);

            if ([importedPresets isKindOfClass:[NSDictionary class]])
            {
                int hb_major, hb_minor, hb_micro;
                int major;

                hb_presets_current_version(&hb_major, &hb_minor, &hb_micro);
                major = [importedPresets[@"VersionMajor"] intValue];

                if (major <= hb_major)
                {
                    presetsArray = importedPresets[@"PresetList"];
                }
                else
                {
                    // Change in major indicates non-backward compatible preset changes.
                    if (outError)
                    {
                        *outError = [self newerPresetErrorForUrl:url];
                    }
                    return nil;
                }
            }
            else if ([importedPresets isKindOfClass:[NSArray class]])
            {
                presetsArray = importedPresets;
            }
        }
    }

    // Convert the array to a HBPreset tree.
    if (presetsArray.count)
    {
        self = [self initWithCategoryName:@"Imported Presets" builtIn:NO];

        if (self)
        {
            for (NSDictionary *dict in presetsArray)
            {
                HBPreset *preset = [[HBPreset alloc] initWithDictionary:dict];
                [self.children addObject:preset];
            }
        }
        [self resetBuiltInAndDefaultState];
        return self;
    }
    else if (outError)
    {
        *outError = [self invalidPresetErrorForUrl:url];
    }

    return nil;
}

- (NSError *)invalidPresetErrorForUrl:(NSURL *)url
{
    NSString *description = [NSString stringWithFormat:HBKitLocalizedString(@"The preset \"%@\" could not be imported.", @"Preset -> import error description"),
                             url.lastPathComponent];
    NSString *reason = HBKitLocalizedString(@"The selected preset is invalid.", @"Preset -> import error reason");

    return [NSError errorWithDomain:@"HBPresetDomain" code:1 userInfo:@{NSLocalizedDescriptionKey: description,
                                                                        NSLocalizedRecoverySuggestionErrorKey: reason}];

}

- (NSError *)newerPresetErrorForUrl:(NSURL *)url
{
    NSString *description = [NSString stringWithFormat:HBKitLocalizedString(@"The preset \"%@\" could not be imported.", @"Preset -> import error description"),
                             url.lastPathComponent];
    NSString *reason = HBKitLocalizedString(@"The selected preset was created with a newer version of HandBrake.",  @"Preset -> import error reason");

    return [NSError errorWithDomain:@"HBPresetDomain" code:2 userInfo:@{NSLocalizedDescriptionKey: description,
                                                                        NSLocalizedRecoverySuggestionErrorKey: reason}];

}

/**
 *  A dictionary representation of the preset.
 */
- (NSDictionary *)dictionary
{
    NSMutableDictionary *output = [[NSMutableDictionary alloc] init];
    [output addEntriesFromDictionary:self.content];

    output[@"PresetName"] = self.name;
    output[@"PresetDescription"] = self.presetDescription;
    output[@"Folder"] = [NSNumber numberWithBool:!self.isLeaf];
    output[@"Type"] = @(!self.isBuiltIn);
    output[@"Default"] = @(self.isDefault);

    if (!self.isLeaf)
    {
        NSMutableArray *childArray = [[NSMutableArray alloc] init];
        for (HBPreset *child in self.children)
        {
            [childArray addObject:[child dictionary]];
        }

        output[@"ChildrenArray"] = childArray;
    }

    return output;
}

- (BOOL)writeToURL:(NSURL *)url atomically:(BOOL)atomically removeRoot:(BOOL)removeRoot error:(NSError * __autoreleasing *)outError
{
    BOOL success = NO;
    NSArray *presetList;

    if (removeRoot)
    {
        presetList = self.dictionary[@"ChildrenArray"];
    }
    else
    {
        presetList = @[self.dictionary];
    }

    int major, minor, micro;
    hb_presets_current_version(&major, &minor, &micro);

    NSDictionary *dict = @{ @"PresetList": presetList,
                            @"VersionMajor": @(major),
                            @"VersionMinor": @(minor),
                            @"VersionMicro": @(micro) };


    NSData *jsonPreset = [NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:NULL];
    success = [jsonPreset writeToURL:url options:NSDataWritingAtomic error:outError];

    return success;
}

- (void)cleanUp
{
    // Run the libhb clean function
    NSString *presetJson = [NSJSONSerialization HB_StringWithJSONObject:self.dictionary options:0 error:NULL];

    if (presetJson.length)
    {
        char *cleanedJson = hb_presets_clean_json(presetJson.UTF8String);
        NSDictionary *cleanedDict = [NSJSONSerialization HB_JSONObjectWithUTF8String:cleanedJson options:0 error:NULL];
        free(cleanedJson);

        if ([cleanedDict isKindOfClass:[NSDictionary class]])
        {
            self.content = [cleanedDict mutableCopy];
        }
    }
}

- (void)resetBuiltInAndDefaultState
{
    _isBuiltIn = NO;
    _isDefault = NO;

    for (HBPreset *child in self.children)
    {
        [child resetBuiltInAndDefaultState];
    }
}

#pragma mark - NSCopying

- (id)copyWithZone:(NSZone *)zone
{
    HBPreset *node = [super copyWithZone:zone];
    node->_name = [self.name copy];
    node->_content = [self.content mutableCopy];
    node->_presetDescription = [self.presetDescription copy];

    return node;
}

- (id)mutableCopyWithZone:(NSZone *)zone
{
    return [[HBMutablePreset allocWithZone:zone] initWithPreset:self];
}

- (NSUInteger)hash
{
    return self.name.hash + self.isBuiltIn + self.isLeaf;
}

#pragma mark - Properties

- (void)setName:(NSString *)name
{
    _name = [name copy];
    [self.delegate nodeDidChange:self];
}

- (void)setIsDefault:(BOOL)isDefault
{
    _isDefault = isDefault;
    [self.delegate nodeDidChange:self];
}

- (BOOL)isSupported
{
    return [_content[@"PresetDisabled"] boolValue] == NO;
}

#pragma mark - Keys

- (id)objectForKey:(NSString *)key
{
    return [_content objectForKey:key];
}

- (nullable id)objectForKeyedSubscript:(NSString *)key
{
    return _content[key];
}

#pragma mark - KVC

- (BOOL)validateName:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    // Return an error is the name is empty
    if (![*ioValue length])
    {
        if (outError)
        {
            *outError = [[NSError alloc] initWithDomain:@"HBErrorDomain"
                                                   code:0
                                               userInfo:@{NSLocalizedDescriptionKey:@"The preset title cannot be empty.",
                                                          NSLocalizedRecoverySuggestionErrorKey:@"Please enter a title."}];
        }
        return NO;
    }

    return YES;
}

@end
