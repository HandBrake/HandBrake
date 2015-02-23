/*  HBPresets.m $
 
 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPresetsManager.h"
#import "HBPreset.h"

#import "HBUtilities.h"

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

- (void)dealloc
{
    [_fileURL release];
    [_defaultPreset release];

    [_root release];

    [super dealloc];
}

- (NSIndexPath *)indexPathOfPreset:(HBPreset *)preset
{
    __block NSIndexPath *retValue = nil;

    // Visit the whole tree to find the index path.
    [self.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop)
    {
        if ([obj isEqualTo:preset])
        {
            retValue = [idx retain];
            *stop = YES;
        }
    }];

    return [retValue autorelease];
}

#pragma mark - HBTreeNode delegate

- (void)nodeDidChange
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBPresetsChangedNotification object:nil];
}

#pragma mark - Load/Save

- (BOOL)loadPresetsFromURL:(NSURL *)url
{
    NSArray *presetsArray = [[NSArray alloc] initWithContentsOfURL:url];

    for (NSDictionary *child in presetsArray)
    {
        [self.root.children addObject:[self loadFromDict:child]];
    }

    [presetsArray release];

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

    if (self.defaultPreset == nil)
    {
        [self selectNewDefault];
    }

    return YES;
}

- (BOOL)savePresetsToURL:(NSURL *)url
{
    NSMutableArray *presetsArray = [[[NSMutableArray alloc] init] autorelease];

    for (HBPreset *node in self.root.children)
    {
        [presetsArray addObject:[self convertToDict:node]];
    }

    return [presetsArray writeToURL:url atomically:YES];
    
    return YES;
}

- (BOOL)savePresets
{
    return [self savePresetsToURL:self.fileURL];
}

#pragma mark - NSDictionary conversions

/**
 *  Converts the NSDictionary to a HBPreset object,
 */
- (HBPreset *)loadFromDict:(NSDictionary *)dict
{
    HBPreset *node = nil;
    if ([dict[@"Folder"] boolValue])
    {
        node = [[[HBPreset alloc] initWithFolderName:dict[@"PresetName"]
                                              builtIn:![dict[@"Type"] boolValue]] autorelease];

        for (NSDictionary *child in dict[@"ChildrenArray"])
        {
            [node.children addObject:[self loadFromDict:child]];
        }
    }
    else
    {
        node = [[[HBPreset alloc] initWithName:dict[@"PresetName"]
                                        content:dict
                                        builtIn:![dict[@"Type"] boolValue]] autorelease];
        node.isDefault = [dict[@"Default"] boolValue];

        if ([dict[@"Default"] boolValue])
        {
            self.defaultPreset = node;
        }
    }

    node.delegate = self;

    return node;
}

/**
 *  Converts the HBPreset and its childrens to a NSDictionary.
 */
- (NSDictionary *)convertToDict:(HBPreset *)node
{
    NSMutableDictionary *output = [[NSMutableDictionary alloc] init];
    [output addEntriesFromDictionary:node.content];

    output[@"PresetName"] = node.name;
    output[@"Folder"] = @(!node.isLeaf);
    output[@"Type"] = @(!node.isBuiltIn);
    output[@"Default"] = @(node.isDefault);

    if (!node.isLeaf)
    {
        NSMutableArray *childArray = [[NSMutableArray alloc] init];
        for (HBPreset *child in node.children)
        {
            [childArray addObject:[self convertToDict:child]];
        }

        output[@"ChildrenArray"] = childArray;
        [childArray release];
    }

    return [output autorelease];
}

#pragma mark - Presets Management

- (BOOL)checkBuiltInsForUpdates
{
    __block BOOL retValue = NO;

    [self.root enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop) {
        NSDictionary *dict = [obj content];

        if ([obj isBuiltIn] && [obj isLeaf])
        {
            if (!dict[@"PresetBuildNumber"] ||
                [dict[@"PresetBuildNumber"] intValue] < [[[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"] intValue])
            {
                retValue = YES;
                *stop = YES;
            }
        }
    }];

    return retValue;
}

- (void)addPresetFromDictionary:(NSDictionary *)preset
{
    HBPreset *presetNode = [[HBPreset alloc] initWithName:preset[@"PresetName"]
                                                   content:preset
                                                   builtIn:NO];

    [self.root insertObject:presetNode inChildrenAtIndex:[self.root countOfChildren]];
    [presetNode release];

    [self savePresets];
}

- (void)addPreset:(HBPreset *)preset
{
    [self.root insertObject:preset inChildrenAtIndex:[self.root countOfChildren]];

    [self savePresets];
}

- (void)deletePresetAtIndexPath:(NSIndexPath *)idx
{
    HBPreset *parentNode = self.root;

    // Find the preset parent array
    // and delete it.
    NSUInteger currIdx = 0;
    NSUInteger i = 0;
    for (i = 0; i < idx.length - 1; i++)
    {
        currIdx = [idx indexAtPosition:i];

        if (parentNode.children.count > currIdx)
        {
            parentNode = [parentNode.children objectAtIndex:currIdx];
        }
    }

    currIdx = [idx indexAtPosition:i];

    if (parentNode.children.count > currIdx)
    {
        if ([[parentNode.children objectAtIndex:currIdx] isDefault])
        {
            [parentNode removeObjectFromChildrenAtIndex:currIdx];
            // Try to select a new default preset
            [self selectNewDefault];
        }
        else
        {
            [parentNode removeObjectFromChildrenAtIndex:currIdx];
        }
    }
}

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
            *stop = YES;
        }

        if ([obj isDefault]) {
            defaultAlreadySetted = YES;
        }
    }];

    if (defaultAlreadySetted) {
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
    else if (firstBuiltInPreset) {
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
            [_defaultPreset autorelease];
        }
        defaultPreset.isDefault = YES;
        _defaultPreset = [defaultPreset retain];

        [self nodeDidChange];
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
    [self deleteBuiltInPresets];

    // Load the built-in presets from the app bundle Resources folder.
    NSURL *fileURL = [[NSBundle mainBundle] URLForResource:@"presets" withExtension:@".plist"];
    NSArray *presetsArray = [[NSArray alloc] initWithContentsOfURL:fileURL];

    for (NSDictionary *child in presetsArray.reverseObjectEnumerator)
    {
        HBPreset *preset = [self loadFromDict:child];

        [preset enumerateObjectsUsingBlock:^(id obj, NSIndexPath *idx, BOOL *stop)
        {
            NSMutableDictionary *presetDict = [[obj content] mutableCopy];

            // Set the current version in the built-in presets, to they won't be reupdated
            // each time the app checks the version.
            presetDict[@"PresetBuildNumber"] = [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];
            [obj setContent:presetDict];
            [presetDict release];
        }];

        [self.root insertObject:preset inChildrenAtIndex:0];
    }

    [presetsArray release];

    // set a new Default preset
    [self selectNewDefault];

    [HBUtilities writeToActivityLog: "built in presets updated to build number: %d", [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]];
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
    [nodeToRemove release];
    [self didChangeValueForKey:@"root"];
}

@end
