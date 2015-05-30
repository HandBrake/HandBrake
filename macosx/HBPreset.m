/*  HBPreset.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPreset.h"
#include "preset.h"

#import "NSJSONSerialization+HBAdditions.h"

@implementation HBPreset

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _name = @"New Preset";
        _presetDescription = @"";
        self.isLeaf = YES;
    }
    return self;
}

- (instancetype)initWithName:(NSString *)title content:(NSDictionary *)content builtIn:(BOOL)builtIn;
{
    self = [self init];
    if (self)
    {
        _name = [title copy];
        _isBuiltIn = builtIn;
        _content = [content copy];
        if ([content[@"PresetDescription"] isKindOfClass:[NSString class]])
        {
            _presetDescription = [content[@"PresetDescription"] copy];
        }
    }
    return self;
}

- (instancetype)initWithFolderName:(NSString *)title builtIn:(BOOL)builtIn;
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

    if ([dict[@"Folder"] boolValue])
    {
        self = [self initWithFolderName:dict[@"PresetName"] builtIn:![dict[@"Type"] boolValue]];

        for (NSDictionary *childDict in [dict[@"ChildrenArray"] reverseObjectEnumerator])
        {
            HBPreset *childPreset = [[HBPreset alloc] initWithDictionary:childDict];
            [self insertObject:childPreset inChildrenAtIndex:0];
        }
    }
    else
    {
        self = [self initWithName:dict[@"PresetName"]
                          content:dict
                          builtIn:![dict[@"Type"] boolValue]];
        self.isDefault = [dict[@"Default"] boolValue];
    }

    return self;
}

- (nullable instancetype)initWithContentsOfURL:(NSURL *)url
{
    NSArray *presetsArray;
    NSString *presetsJson;

    if ([url.pathExtension isEqualToString:@"json"])
    {
        NSData *data = [[NSData alloc] initWithContentsOfURL:url];
        presetsJson = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    }
    else
    {
        NSArray *array = [[NSArray alloc] initWithContentsOfURL:url];
        if ([NSJSONSerialization isValidJSONObject:array])
        {
            presetsJson = [NSJSONSerialization HB_StringWithJSONObject:array options:0 error:NULL];
        }
    }

    // Run the json through the libhb import function
    // to avoid importing unknowns keys.
    if (presetsJson.length) {
        char *importedJson = hb_presets_import_json(presetsJson.UTF8String);

        if (importedJson)
        {
            id importedPresets = [NSJSONSerialization HB_JSONObjectWithUTF8String:importedJson options:0 error:NULL];

            if ([importedPresets isKindOfClass:[NSDictionary class]])
            {
                presetsArray = importedPresets[@"PresetList"];
            }
            else if ([importedPresets isKindOfClass:[NSArray class]])
            {
                presetsArray = importedPresets;
            }
        }

        free(importedJson);
    }

    if (presetsArray.count)
    {
        self = [self initWithFolderName:@"Imported Presets" builtIn:NO];

        if (self)
        {
            for (NSDictionary *dict in presetsArray)
            {
                HBPreset *preset = [[HBPreset alloc] initWithDictionary:dict];
                [self.children addObject:preset];
            }
        }
        return self;
    }

    return nil;
}

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

- (BOOL)writeToURL:(NSURL *)url atomically:(BOOL)atomically format:(HBPresetFormat)format removeRoot:(BOOL)removeRoot;
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
                            @"VersionMicro": @(minor),
                            @"VersionMinor": @(micro) };

    if (format == HBPresetFormatPlist)
    {
        success = [dict writeToURL:url atomically:atomically];
    }
    else
    {
        NSData *jsonPreset = [NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted error:NULL];
        success = [jsonPreset writeToURL:url atomically:atomically];
    }

    return success;
}

- (id)copyWithZone:(NSZone *)zone
{
    HBPreset *node = [super copyWithZone:zone];
    node->_name = [self.name copy];
    node->_content = [self.content copy];
    node->_presetDescription = [self.presetDescription copy];

    return node;
}

- (NSUInteger)hash
{
    return self.name.hash + self.isBuiltIn + self.isLeaf;
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
            self.content = cleanedDict;
        }
    }
}

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
