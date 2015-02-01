/*  HBRange.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRange.h"
#import "HBTitle.h"
#import "NSCodingMacro.h"

NSString *HBRangeChangedNotification = @"HBRangeChangedNotification";

@implementation HBRange

#pragma mark - NSCoding

- (instancetype)initWithTitle:(HBTitle *)title
{
    self = [super init];
    if (self)
    {
        _title = title;

        _chapterStart = 0;
        _chapterStop = (int)title.chapters.count - 1;

        _secondsStart = 0;
        _secondsStop = title.duration;

        _frameStart = 0;
        _frameStop = title.frames;
    }
    return self;
}

- (void)postChangedNotification
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBRangeChangedNotification object:self];
}

- (void)setChapterStart:(int)chapterStart
{
    if (chapterStart > self.chapterStop)
    {
        self.chapterStop = chapterStart;
    }

    _chapterStart = chapterStart;

    [self postChangedNotification];
}

- (void)setChapterStop:(int)chapterStop
{
    if (chapterStop < self.chapterStart)
    {
        self.chapterStart = chapterStop;
    }

    _chapterStop = chapterStop;

    [self postChangedNotification];
}

- (NSString *)duration
{
    if (self.type == HBRangeTypeChapters)
    {
        hb_title_t *title = self.title.hb_title;
        hb_chapter_t *chapter;
        int64_t duration = 0;

        for (int i = self.chapterStart; i <= self.chapterStop; i++ )
        {
            chapter = (hb_chapter_t *) hb_list_item(title->list_chapter, i);
            duration += chapter->duration;
        }

        duration /= 90000; // pts -> seconds
        return [NSString stringWithFormat: @"%02lld:%02lld:%02lld", duration / 3600, ( duration / 60 ) % 60,  duration % 60];
    }
    else if (self.type == HBRangeTypeSeconds)
    {
        int duration = self.secondsStop - self.secondsStart;
        return [NSString stringWithFormat:@"%02d:%02d:%02d", duration / 3600, (duration / 60) % 60, duration % 60];

    }
    else if (self.type == HBRangeTypeFrames)
    {
        hb_title_t *title = self.title.hb_title;
        int duration = (self.frameStop - self.frameStart) / (title->vrate.num / title->vrate.den);
        return [NSString stringWithFormat: @"%02d:%02d:%02d", duration / 3600, ( duration / 60 ) % 60, duration % 60];
    }

    return @"00:00:00";
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    if ([key isEqualToString:@"duration"])
    {
        retval = [NSSet setWithObjects:@"type", @"chapterStart", @"chapterStop", @"frameStart", @"frameStop",
                  @"secondsStart", @"secondsStop",nil];
    }

    if ([key isEqualToString:@"chaptersSelected"] ||
        [key isEqualToString:@"secondsSelected"] ||
        [key isEqualToString:@"framesSelected"])
    {
        retval = [NSSet setWithObjects:@"type",nil];

    }

    return retval;
}

- (void)setNilValueForKey:(NSString *)key
{
    [self setValue:@0 forKey:key];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBRange *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_type = _type;
        copy->_chapterStart = _chapterStart;
        copy->_chapterStop = _chapterStop;
        copy->_secondsStart = _secondsStart;
        copy->_secondsStop = _secondsStop;
        copy->_frameStart = _frameStart;
        copy->_frameStop = _frameStop;
    }

    return copy;
}

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBRangeVersion"];

    encodeInt(_type);

    encodeInt(_chapterStart);
    encodeInt(_chapterStop);

    encodeInt(_secondsStart);
    encodeInt(_secondsStop);

    encodeInt(_frameStart);
    encodeInt(_frameStop);
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_type);

    decodeInt(_chapterStart);
    decodeInt(_chapterStop);

    decodeInt(_secondsStart);
    decodeInt(_secondsStop);

    decodeInt(_frameStart);
    decodeInt(_frameStop);
    
    return self;
}

@end
