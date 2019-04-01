/*  HBChapter.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBChapter.h"
#import "HBCodingUtilities.h"

@implementation HBChapter

- (instancetype)init
{
    self = [self initWithTitle:@"No Value" index:0 timestamp:0 duration:0];
    return self;
}

- (instancetype)initWithTitle:(NSString *)title index:(NSUInteger)idx timestamp:(uint64_t)timestamp duration:(uint64_t)duration
{
    NSParameterAssert(title);

    self = [super init];
    if (self)
    {
        _timestamp = timestamp;
        _duration = duration;
        _title = [title copy];
        _index = idx;

    }
    return self;
}

#pragma mark - Properties

- (void)setTitle:(NSString *)title
{
    if (![title isEqualToString:_title])
    {
        [[self.undo prepareWithInvocationTarget:self] setTitle:_title];
    }
    if (title == nil)
    {
        _title = @"";
    }
    else
    {
        _title = title;
    }
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBChapter *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_title = [_title copy];
        copy->_timestamp = _timestamp;
        copy->_duration = _duration;
        copy->_index = _index;
    }

    return copy;
}


#pragma mark - NSCoding

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBChapterVersion"];

    encodeObject(_title);
    encodeInteger(_timestamp);
    encodeInteger(_duration);
    encodeInteger(_index);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    int version = [decoder decodeIntForKey:@"HBChapterVersion"];

    if (version == 1 && (self = [self init]))
    {
        decodeObjectOrFail(_title, NSString);
        decodeInteger(_timestamp);
        decodeInteger(_duration);
        decodeInteger(_index);

        return self;
    }

fail:
    return nil;
}

@end
