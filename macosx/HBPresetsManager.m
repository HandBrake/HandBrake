/*  HBPresets.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPresetsManager.h"
#import "HBPreset.h"

#import "HBUtilities.h"
#import "NSJSONSerialization+HBAdditions.h"

#include "handbrake/preset.h"

NSString *HBPresetsChangedNotification = @"HBPresetsChangedNotification";

@interface HBPresetsManager () <HBTreeNodeDelegate>

@property (nonatomic, readonly, copy) NSURL *fileURL;

@end

@implementation HBPresetsManager

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        // Init the root of the tree, it won't never be shown in the UI
        _root = [[HBPreset alloc] initWithCategoryName:@"Root" builtIn:YES];
        _root.delegate = self;
    }
    return self;
}

- (instancetype)initWithURL:(NSURL *)url
{
    self = [self init];
    if (self)
    {
        _fileURL = [url copy];
        [self loadPresetsFromURL:url];
    }
    return self;
}

#pragma mark - HBTreeNode delegate

- (void)nodeDidChange:(HBTreeNode *)node
{
    [NSNotificationCenter.defaultCenter postNotificationName:HBPresetsChangedNotification object:self];
}

- (void)treeDidRemoveNode:(HBTreeNode *)node
{
    if (node == self.defaultPreset)
    {
        // Select a new default preset
        [self selectNewDefault];
    }
}

#pragma mark - Load/Save

- (BOOL)isNewer:(NSDictionary *)dict
{
    int major, minor, micro;
    hb_presets_current_version(&major, &minor, &micro);

    if ([dict[@"VersionMajor"] intValue] > major ||
        ([dict[@"VersionMajor"] intValue] == major && [dict[@"VersionMinor"] intValue] > minor) ||
        ([dict[@"VersionMajor"] intValue] == major && [dict[@"VersionMinor"] intValue] == minor && [dict[@"VersionMicro"] intValue] > micro))
    {
        return YES;
    }

    return NO;
}

- (BOOL)isOutOfDate:(NSDictionary *)dict
{
    int major, minor, micro;
    hb_presets_current_version(&major, &minor, &micro);

    if (major != [dict[@"VersionMajor"] intValue] ||
        minor != [dict[@"VersionMinor"] intValue] ||
        micro != [dict[@"VersionMicro"] intValue])
    {
        return YES;
    }
    return NO;
}

- (NSURL *)backupURLfromURL:(NSURL *)url versionMajor:(int)major minor:(int)minor micro:(int)micro
{
    NSString *backupName = [NSString stringWithFormat:@"%@.%d.%d.%d.json",
                            url.lastPathComponent.stringByDeletingPathExtension,
                            major, minor, micro];

    return [url.URLByDeletingLastPathComponent URLByAppendingPathComponent:backupName isDirectory:NO];
}

typedef NS_ENUM(NSUInteger, HBPresetLoadingResult) {
    HBPresetLoadingResultOK,
    HBPresetLoadingResultOKUpgraded,
    HBPresetLoadingResultFailedNewer,
    HBPresetLoadingResultFailed
};

- (NSDictionary *)dictionaryWithPresetsAtURL:(NSURL *)url backup:(BOOL)backup result:(HBPresetLoadingResult *)result
{
    NSData *presetData = [[NSData alloc] initWithContentsOfURL:url];

    if (presetData)
    {
        const char *json = [[NSString alloc] initWithData:presetData encoding:NSUTF8StringEncoding].UTF8String;
        NSDictionary *presetsDict = [NSJSONSerialization HB_JSONObjectWithUTF8String:json options:0 error:NULL];

        if ([presetsDict isKindOfClass:[NSDictionary class]])
        {
            if ([self isNewer:presetsDict])
            {
                *result = HBPresetLoadingResultFailedNewer;
                return nil;
            }
            else if ([self isOutOfDate:presetsDict])
            {
                // There is a new presets file format,
                // make a backup of the old one for the previous versions.
                if (backup)
                {
                    NSURL *backupURL = [self backupURLfromURL:self.fileURL
                                                 versionMajor:[presetsDict[@"VersionMajor"] intValue]
                                                        minor:[presetsDict[@"VersionMinor"] intValue]
                                                        micro:[presetsDict[@"VersionMicro"] intValue]];
                    [NSFileManager.defaultManager copyItemAtURL:url toURL:backupURL error:NULL];
                }

                // Update the presets to the current format.
                char *updatedJson;
                hb_presets_import_json(json, &updatedJson);
                presetsDict = [NSJSONSerialization HB_JSONObjectWithUTF8String:updatedJson options:0 error:NULL];
                free(updatedJson);

                *result = HBPresetLoadingResultOKUpgraded;
            }
            else
            {
                // Else, clean up the presets just to be sure
                // it's in the right format.
                char *cleanedJson = hb_presets_clean_json(json);
                presetsDict = [NSJSONSerialization HB_JSONObjectWithUTF8String:cleanedJson options:0 error:NULL];
                free(cleanedJson);

                *result = HBPresetLoadingResultOK;
            }

            return presetsDict;
        }
    }

    *result = HBPresetLoadingResultFailed;
    return nil;
}

- (void)loadPresetsFromURL:(NSURL *)url
{
    HBPresetLoadingResult result;
    NSDictionary *presetsDict;

    // Load the presets
    presetsDict = [self dictionaryWithPresetsAtURL:url backup:YES result:&result];

    if (result == HBPresetLoadingResultFailedNewer)
    {
        // Try a backup
        int major, minor, micro;
        hb_presets_current_version(&major, &minor, &micro);

        NSURL *backupURL = [self backupURLfromURL:url versionMajor:major minor:minor micro:micro];

        HBPresetLoadingResult backupResult;
        presetsDict = [self dictionaryWithPresetsAtURL:backupURL backup:NO result:&backupResult];

        // Change the fileURL so we don't overwrite
        // the latest version presets.
        _fileURL = backupURL;
    }

    // Add the presets to the root
    if (presetsDict)
    {
        for (NSDictionary *child in presetsDict[@"PresetList"])
        {
            [self.root insertObject:[[HBPreset alloc] initWithDictionary:child] inChildrenAtIndex:self.root.countOfChildren];
        }

        if (result == HBPresetLoadingResultOKUpgraded)
        {
            [self generateBuiltInPresets];
        }
    }

    // If the preset list contains no leaf,
    // add back the built in presets.
    __block BOOL leafFound = NO;
    [self.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop) {
        if ([obj isLeaf])
        {
            leafFound = YES;
            *stop = YES;
        }
    }];

    if (!leafFound)
    {
        [self generateBuiltInPresets];
    }

    [self selectNewDefault];

    if (result == HBPresetLoadingResultOKUpgraded)
    {
        [self savePresets];
    }
}

- (BOOL)savePresetsToURL:(NSURL *)url
{
    return [self.root writeToURL:url atomically:YES removeRoot:YES error:NULL];
}

- (BOOL)savePresets
{
    return [self savePresetsToURL:self.fileURL];
}

#pragma mark - Presets Management

- (void)addPreset:(HBPreset *)preset
{
    // Make sure no preset has the default flag enabled.
    [preset enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop) {
        [obj setIsDefault:NO];
    }];

    [self.root insertObject:preset inChildrenAtIndex:[self.root countOfChildren]];

    [self savePresets];
}

- (void)deletePresetAtIndexPath:(NSIndexPath *)idx
{
    [self.root removeObjectAtIndexPath:idx];
}

- (void)replacePresetAtIndexPath:(NSIndexPath *)idx withPreset:(HBPreset *)preset
{
    [self.root replaceObjectAtIndexPath:idx withObject:preset];
}

- (NSIndexPath *)indexPathOfPreset:(HBPreset *)preset
{
    return [self.root indexPathOfObject:preset];
}

#pragma mark - Default preset

/**
 *  Private method to select a new default after the default preset is deleted
 *  or when the built-in presets are regenerated.
 */
- (void)selectNewDefault
{
    __block HBPreset *normalPreset = nil;
    __block HBPreset *firstUserPreset = nil;
    __block HBPreset *firstBuiltInPreset = nil;
    __block BOOL defaultAlreadySet = NO;

    // Search for a possible new default preset
    // Try to use "Normal" or the first user preset.
    [self.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop) {
        if ([obj isBuiltIn] && [obj isLeaf])
        {
            if ([[obj name] isEqualToString:@"Fast 1080p30"])
            {
                normalPreset = obj;
            }
            if (firstBuiltInPreset == nil)
            {
                firstBuiltInPreset = obj;
            }
        }
        else if ([obj isLeaf] && firstUserPreset == nil)
        {
            firstUserPreset = obj;
        }

        if ([obj isDefault])
        {
            self.defaultPreset = obj;
            defaultAlreadySet = YES;
        }

        if ([obj isDefault])
        {
            [obj setIsDefault:NO];
        }
    }];

    if (defaultAlreadySet)
    {
        self.defaultPreset.isDefault = YES;
        return;
    }
    else if (normalPreset)
    {
        self.defaultPreset = normalPreset;
        normalPreset.isDefault = YES;
    }
    else if (firstUserPreset)
    {
        self.defaultPreset = firstUserPreset;
        firstUserPreset.isDefault = YES;
    }
    else if (firstBuiltInPreset)
    {
        self.defaultPreset = firstBuiltInPreset;
        firstBuiltInPreset.isDefault = YES;
    }
}

- (void)setDefaultPreset:(HBPreset *)defaultPreset
{
    if (defaultPreset && defaultPreset.isLeaf)
    {
        if (_defaultPreset)
        {
            _defaultPreset.isDefault = NO;
        }
        defaultPreset.isDefault = YES;
        _defaultPreset = defaultPreset;
    }
}

#pragma mark - Built In Generation

/**
 * Built-in preset categories at the root of the hierarchy
 */
- (void)generateBuiltInPresets
{
    // Load the built-in presets from libhb.
    const char *presets = hb_presets_builtin_get_json();
    NSData *presetsData = [NSData dataWithBytes:presets length:strlen(presets)];

    NSError *error = nil;
    NSArray *presetsArray = [NSJSONSerialization JSONObjectWithData:presetsData options:NSJSONReadingAllowFragments error:&error];

    if (presetsArray.count == 0)
    {
        [HBUtilities writeToActivityLog:"failed to update built-in presets"];

        if (error)
        {
            [HBUtilities writeToActivityLog:"Error raised:\n%s", error.localizedFailureReason];
        }
    }
    else
    {
        [self deleteBuiltInPresets];

        for (NSDictionary *child in presetsArray.reverseObjectEnumerator)
        {
            HBPreset *preset = [[HBPreset alloc] initWithDictionary:child];
            [self.root insertObject:preset inChildrenAtIndex:0];
        }

        // set a new Default preset
        [self selectNewDefault];

        [HBUtilities writeToActivityLog: "built-in presets updated"];
    }
}

- (void)deleteBuiltInPresets
{
    [self willChangeValueForKey:@"root"];
    NSMutableArray *nodeToRemove = [[NSMutableArray alloc] init];
    for (HBPreset *node in self.root.children)
    {
        if (node.isBuiltIn)
        {
            [nodeToRemove addObject:node];
        }
    }
    [self.root.children removeObjectsInArray:nodeToRemove];
    [self didChangeValueForKey:@"root"];
}

@end
