//
//  HBSecurityAccessToken.m
//  HandBrake
//
//  Created by Damiano Galassi on 24/01/17.
//
//

#import "HBSecurityAccessToken.h"

@interface HBSecurityAccessToken ()
@property (nonatomic, readonly) id<HBSecurityScope> object;
@property (nonatomic, readonly) BOOL accessed;
@end

@implementation HBSecurityAccessToken

- (instancetype)initWithObject:(id<HBSecurityScope>)object;
{
    self = [super init];
    if (self)
    {
        _object = object;
        _accessed = [_object startAccessingSecurityScopedResource];
    }
    return self;
}

+ (instancetype)tokenWithObject:(id<HBSecurityScope>)object
{
    return [[self alloc] initWithObject:object];
}

- (void)dealloc
{
    if (_accessed)
    {
        [_object stopAccessingSecurityScopedResource];
    }
}

@end
