/*  HBRange.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBRange.h"
#import "HBTitle+Private.h"
#import "HBCodingUtilities.h"

NSString *HBRangeChangedNotification = @"HBRangeChangedNotification";

@implementation HBRange

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
    [NSNotificationCenter.defaultCenter postNotificationName:HBRangeChangedNotification object:self];
}

- (void)setType:(HBRangeType)type
{
    if (type != _type)
    {
        [[self.undo prepareWithInvocationTarget:self] setType:_type];
    }
    _type = type;
}

- (void)setChapterStart:(int)chapterStart
{
    if (chapterStart != _chapterStart)
    {
        [[self.undo prepareWithInvocationTarget:self] setChapterStart:_chapterStart];
    }

    if (chapterStart > self.chapterStop)
    {
        self.chapterStop = chapterStart;
    }

    _chapterStart = chapterStart;

    [self postChangedNotification];
}

- (void)setChapterStop:(int)chapterStop
{
    if (chapterStop != _chapterStop)
    {
        [[self.undo prepareWithInvocationTarget:self] setChapterStop:_chapterStop];
    }

    if (chapterStop < self.chapterStart)
    {
        self.chapterStart = chapterStop;
    }

    _chapterStop = chapterStop;

    [self postChangedNotification];
}

- (void)setFrameStart:(int)frameStart
{
    if (frameStart != _frameStart)
    {
        [[self.undo prepareWithInvocationTarget:self] setFrameStart:_frameStart];
    }
    _frameStart = frameStart;
}

- (void)setFrameStop:(int)frameStop
{
    if (frameStop != _frameStop)
    {
        [[self.undo prepareWithInvocationTarget:self] setFrameStop:_frameStop];
    }
    _frameStop = frameStop;
}

- (void)setSecondsStart:(int)secondsStart
{
    if (secondsStart != _secondsStart)
    {
        [[self.undo prepareWithInvocationTarget:self] setSecondsStart:_secondsStart];
    }
    _secondsStart = secondsStart;
}

- (void)setSecondsStop:(int)secondsStop
{
    if (secondsStop != _secondsStop)
    {
        [[self.undo prepareWithInvocationTarget:self] setSecondsStop:_secondsStop];
    }
    _secondsStop = secondsStop;
}

- (NSString *)duration
{
    if (self.type == HBRangeTypeChapters)
    {
        int64_t duration = 0;
        hb_title_t *title = self.title.hb_title;

        for (int i = self.chapterStart; i <= self.chapterStop; i++ )
        {
            hb_chapter_t *chapter = (hb_chapter_t *)hb_list_item(title->list_chapter, i);
            if (chapter)
            {
                duration += chapter->duration;
            }
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
        int duration = (int) ((self.frameStop - self.frameStart) / (title->vrate.num / (double)title->vrate.den));
        return [NSString stringWithFormat: @"%02d:%02d:%02d", duration / 3600, ( duration / 60 ) % 60, duration % 60];
    }

    return @"00:00:00";
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingDuration
{
    return [NSSet setWithObjects:@"type", @"chapterStart", @"chapterStop",
                                 @"frameStart", @"frameStop",
                                 @"secondsStart", @"secondsStop",nil];
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

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBRangeVersion"];

    encodeInteger(_type);

    encodeInt(_chapterStart);
    encodeInt(_chapterStop);

    encodeInt(_secondsStart);
    encodeInt(_secondsStop);

    encodeInt(_frameStart);
    encodeInt(_frameStop);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInteger(_type); if (_type < HBRangeTypeChapters || _type > HBRangePreviewIndex) { goto fail; }
    decodeInt(_chapterStart);  if (_chapterStart < 0) { goto fail; }
    decodeInt(_chapterStop);  if (_chapterStop < _chapterStart) { goto fail; }

    decodeInt(_secondsStart);
    decodeInt(_secondsStop);

    decodeInt(_frameStart);
    decodeInt(_frameStop);

    return self;

fail:
    return nil;
}

@end
