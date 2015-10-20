/*  HBChapter.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBChapter.h"
#import "HBCodingUtilities.h"

@implementation HBChapter

- (instancetype)init
{
    self = [self initWithTitle:@"No Value" index:0 duration:0];
    return self;
}

- (instancetype)initWithTitle:(NSString *)title index:(NSUInteger)idx duration:(uint64_t)duration
{
    NSParameterAssert(title);

    self = [super init];
    if (self)
    {
        uint64_t hours    = duration / 90000 / 3600;
        uint64_t minutes  = ((duration / 90000 ) % 3600) / 60;
        uint64_t seconds  = (duration / 90000 ) % 60;

        _duration = [NSString stringWithFormat:@"%02llu:%02llu:%02llu", hours, minutes, seconds];
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
    _title = title;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBChapter *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_title = [_title copy];
        copy->_duration = [_duration copy];
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
    encodeObject(_duration);
    encodeInteger(_index);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    int version = [decoder decodeIntForKey:@"HBChapterVersion"];

    if (version == 1 && (self = [self init]))
    {
        decodeObject(_title, NSString);
        decodeObject(_duration, NSString);
        decodeInteger(_index);

        return self;
    }
    
    return nil;
}

@end
