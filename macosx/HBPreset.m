/*  HBPreset.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

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
        self.isLeaf = NO;
    }
    return self;
}

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

- (void)dealloc
{
    [_name release];
    [_content release];
    [_presetDescription release];

    [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone
{
    HBPreset *node = [[self class] allocWithZone:zone];
    node->_name = [self.name copy];
    node->_content = [self.content copy];
    node->_presetDescription = [self.presetDescription copy];
    for (HBPreset *children in self.children)
    {
        [node.children addObject:[[children copy] autorelease]];
    }

    return node;
}

- (NSUInteger)hash
{
    return self.name.hash + self.isBuiltIn + self.isLeaf;
}

- (void)setName:(NSString *)name
{
    [_name autorelease];
    _name = [name copy];

    [self.delegate nodeDidChange];
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
