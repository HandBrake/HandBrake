/*  HBSecurityAccessToken.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSecurityAccessToken.h"

@interface HBSecurityAccessToken ()
@property (nonatomic, readonly) id<HBSecurityScope> object;
@property (nonatomic, readonly) BOOL accessed;
@end

@implementation HBSecurityAccessToken

- (instancetype)initWithObject:(id<HBSecurityScope>)object
{
    self = [super init];
    if (self)
    {
        _object = object;
        _accessed = [_object startAccessingSecurityScopedResource];
    }
    return self;
}

- (instancetype)initWithAlreadyAccessedObject:(id<HBSecurityScope>)object
{
    self = [super init];
    if (self)
    {
        _object = object;
        _accessed = YES;
    }
    return self;
}

+ (instancetype)tokenWithObject:(id<HBSecurityScope>)object
{
    return [[self alloc] initWithObject:object];
}

+ (instancetype)tokenWithAlreadyAccessedObject:(id<HBSecurityScope>)object
{
    return [[self alloc] initWithAlreadyAccessedObject:object];
}

- (void)dealloc
{
    if (_accessed)
    {
        [_object stopAccessingSecurityScopedResource];
    }
}

@end
