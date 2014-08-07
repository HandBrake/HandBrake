//
//  HBBaseNode.m
//  PresetsView
//
//  Created by Damiano Galassi on 14/07/14.
//  Copyright (c) 2014 Damiano Galassi. All rights reserved.
//

#import "HBPreset.h"

@implementation HBPreset

- (instancetype)initWithName:(NSString *)title content:(NSDictionary *)content builtIn:(BOOL)builtIn;
{
    self = [self init];
    if (self)
    {
        _name = [title copy];
        _isBuiltIn = builtIn;
        _content = [content retain];
        _presetDescription = [content[@"PresetDescription"] copy];
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
        _isLeaf = NO;
    }
    return self;
}

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _children = [[NSMutableArray alloc] init];
        _name = @"New Preset";
        _isLeaf = YES;
        _presetDescription = @"";
    }
    return self;
}

- (void)dealloc
{
    [_name release];
    [_content release];
    [_presetDescription release];
    [_children release];

    [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone
{
    HBPreset *node = [[HBPreset alloc] init];
    node->_name = [self.name copy];
    node->_content = [self.content copy];
    node->_presetDescription = [self.presetDescription copy];
    for (HBPreset *children in self.children)
    {
        [node->_children addObject:[[children copy] autorelease]];
    }

    return node;
}

- (NSUInteger)hash
{
    return self.name.hash + self.isBuiltIn + self.isLeaf;
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

#pragma mark - Enumeration

- (void)enumerateObjectsUsingBlock:(void (^)(id obj, NSIndexPath *idx, BOOL *stop))block
{
    BOOL stop = NO;
    NSMutableArray *queue = [[NSMutableArray alloc] init];
    NSMutableArray *indexesQueue = [[NSMutableArray alloc] init];

    [queue addObject:self];
    [indexesQueue addObject:[[[NSIndexPath alloc] init] autorelease]];

    HBPreset *node = nil;
    while ((node = [queue lastObject]) != nil)
    {
        // Get the index path of the current object
        NSIndexPath *indexPath = [indexesQueue lastObject];

        // Call the block
        block(node, indexPath, &stop);

        if (stop)
        {
            break;
        }

        [indexesQueue removeLastObject];

        for (int i = 0; i < node.children.count; i++)
        {
            [indexesQueue addObject:[indexPath indexPathByAddingIndex:i]];
        }

        [queue removeLastObject];
        [queue addObjectsFromArray:node.children];

    }

    [queue release];
    [indexesQueue release];
}

@end
