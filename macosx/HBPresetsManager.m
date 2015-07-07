/*  HBPresets.m $
 
 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPresetsManager.h"
#import "HBPreset.h"

#import "HBUtilities.h"
#import "NSJSONSerialization+HBAdditions.h"

#include "preset.h"

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
        _root = [[HBPreset alloc] initWithFolderName:@"Root" builtIn:YES];
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

- (void)nodeDidChange:(id)node
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBPresetsChangedNotification object:nil];
}

- (void)treeDidRemoveNode:(id)node
{
    if (node == self.defaultPreset)
    {
        // Select a new default preset
        [self selectNewDefault];
    }
}

#pragma mark - Load/Save

/**
 *  Loads the old presets format (0.10 and earlier) plist
 *
 *  @param url the url of the plist
 */
- (void)loadOldPresetsFromURL:(NSURL *)url
{
    HBPreset *oldPresets = [[HBPreset alloc] initWithContentsOfURL:url];

    for (HBPreset *preset in oldPresets.children)
    {
        [self.root.children addObject:preset];
    }
}

- (BOOL)checkIfOutOfDate:(NSDictionary *)dict
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

- (BOOL)loadPresetsFromURL:(NSURL *)url
{
    NSData *presetData = [[NSData alloc] initWithContentsOfURL:url];

    // Try to load to load the old presets file
    // if the new one is empty
    if (presetData == nil)
    {
        [self loadOldPresetsFromURL:[url.URLByDeletingPathExtension URLByAppendingPathExtension:@"plist"]];
        [self generateBuiltInPresets];
    }
    else
    {
        const char *json = [[NSString alloc] initWithData:presetData encoding:NSUTF8StringEncoding].UTF8String;
        char *cleanedJson = hb_presets_clean_json(json);
        NSDictionary *presetsDict = [NSJSONSerialization HB_JSONObjectWithUTF8String:cleanedJson options:0 error:NULL];

        if ([self checkIfOutOfDate:presetsDict])
        {
            char *updatedJson = hb_presets_import_json(cleanedJson);
            presetsDict = [NSJSONSerialization HB_JSONObjectWithUTF8String:updatedJson options:0 error:NULL];
            free(updatedJson);
        }

        free(cleanedJson);

        for (NSDictionary *child in presetsDict[@"PresetList"])
        {
            [self.root.children addObject:[[HBPreset alloc] initWithDictionary:child]];
        }

        if ([self checkIfOutOfDate:presetsDict])
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

    return YES;
}

- (BOOL)savePresetsToURL:(NSURL *)url
{
    return [self.root writeToURL:url atomically:YES format:HBPresetFormatJson removeRoot:YES];
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
    __block BOOL defaultAlreadySetted = NO;

    // Search for a possibile new default preset
    // Try to use "Normal" or the first user preset.
    [self.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop) {
        if ([obj isBuiltIn] && [obj isLeaf])
        {
            if ([[obj name] isEqualToString:@"Normal"])
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
            defaultAlreadySetted = YES;
        }

        [obj setIsDefault:NO];
    }];

    if (defaultAlreadySetted)
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
 * Built-in preset folders at the root of the hierarchy
 *
 * Note: the built-in presets will *not* sort themselves alphabetically,
 * so they will appear in the order you create them.
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
