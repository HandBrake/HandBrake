/* HBTitleSelectionRange.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBTitleSelectionRange.h"

@interface HBTitleSelectionRange ()

@property (nonatomic) int chaptersCount;
@property (nonatomic) int secondsCount;
@property (nonatomic) int framesCount;

@end

@implementation HBTitleSelectionRange

- (instancetype)initWithTitles:(NSArray<HBTitle *> *)titles
{
    self = [super init];
    if (self)
    {
        for (HBTitle *title in titles)
        {
            if (title.chapters.count > _chaptersCount)
            {
                _chaptersCount = (int)title.chapters.count;
            }
            if (title.duration > _secondsCount)
            {
                _secondsCount = title.duration;
            }
            if (title.frames > _framesCount)
            {
                _framesCount = title.frames;
            }

            _chapterStop = _chaptersCount - 1;
            _frameStop = _framesCount;
            _secondsStop = _secondsCount;
        }
    }
    return self;
}

- (void)setType:(HBTitleSelectionRangeType)type
{
    if (type != _type)
    {
        [(HBTitleSelectionRange *)[self.undo prepareWithInvocationTarget:self] setType:_type];
    }
    _type = type;
}

- (void)setTrim:(HBTitleSelectionTrimType)trim
{
    if (trim != _trim)
    {
        [[self.undo prepareWithInvocationTarget:self] setTrim:_trim];

        if (!(self.undo.isUndoing || self.undo.isRedoing))
        {
            if (trim == HBTitleSelectionTrimTypeNormal)
            {
                self.chapterStop = self.chaptersCount;
                self.secondsStop = self.secondsCount;
                self.frameStop = self.framesCount;
            }
            else
            {
                self.chapterStop = 0;
                self.secondsStop = 0;
                self.frameStop = 0;
            }
        }
    }

    _trim = trim;
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

- (void)setNilValueForKey:(NSString *)key
{
    [self setValue:@0 forKey:key];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBTitleSelectionRange *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_type = _type;
        copy->_trim = _trim;
        copy->_chapterStart = _chapterStart;
        copy->_chapterStop = _chapterStop;
        copy->_secondsStart = _secondsStart;
        copy->_secondsStop = _secondsStop;
        copy->_frameStart = _frameStart;
        copy->_frameStop = _frameStop;
    }

    return copy;
}

@end

@implementation HBTitleSelectionRange (UIAdditions)

- (NSArray<NSString *> *)chapters
{
    NSMutableArray<NSString *> *chapters = [NSMutableArray array];
    for (int i = 0; i < self.chaptersCount; i++)
    {
        [chapters addObject:[NSString stringWithFormat: @"%d", i + 1]];

    }
    return chapters;
}

- (NSArray<NSString *> *)types
{
    return @[NSLocalizedString(@"Chapters", @"HBRange -> display name"),
             NSLocalizedString(@"Seconds", @"HBRange -> display name"),
             NSLocalizedString(@"Frames", @"HBRange -> display name")];
}

- (BOOL)chaptersSelected
{
    return self.type == HBRangeTypeChapters;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingChaptersSelected
{
    return [NSSet setWithObjects:@"type", nil];
}

- (BOOL)secondsSelected
{
    return self.type == HBRangeTypeSeconds;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingSecondsSelected
{
    return [NSSet setWithObjects:@"type", nil];
}

- (BOOL)framesSelected
{
    return self.type == HBRangeTypeFrames;
}

+ (NSSet<NSString *> *)keyPathsForValuesAffectingFramesSelected
{
    return [NSSet setWithObjects:@"type", nil];
}

@end

@implementation HBJob (HBTitleSelectionRangeAdditions)

- (void)applySelectionRange:(nullable HBTitleSelectionRange *)range
{
    HBTitle *title = self.title;

    if (range == nil)
    {
        return;
    }

    if (range.type == HBTitleSelectionRangeTypeChapters)
    {
        self.range.type = HBRangeTypeChapters;
        self.range.chapterStart = range.chapterStart <= self.range.chapterStop ? range.chapterStart : self.range.chapterStart;
        if (range.trim == HBTitleSelectionTrimTypeNormal)
        {
            int chapters = (int)title.chapters.count;
            if (range.chapterStop < chapters && range.chapterStop >= self.range.chapterStart)
            {
                self.range.chapterStop = range.chapterStop;
            }
        }
        else if (range.trim == HBTitleSelectionTrimTypeEnd)
        {
            int chapters = self.range.chapterStop - self.range.chapterStart;
            if (range.chapterStop < chapters)
            {
                self.range.chapterStop = self.range.chapterStop - range.chapterStop - 1;
            }
        }
    }
    else if (range.type == HBTitleSelectionRangeTypeSeconds)
    {
        self.range.type = HBRangeTypeSeconds;
        self.range.secondsStart = range.secondsStart < self.range.secondsStop ? range.secondsStart : self.range.secondsStart;
        if (range.trim == HBTitleSelectionTrimTypeNormal)
        {
            if (range.secondsStop > self.range.secondsStart)
            {
                self.range.secondsStop = range.secondsStop;
            }
        }
        else if (range.trim == HBTitleSelectionTrimTypeEnd)
        {
            int seconds = self.range.secondsStop - self.range.secondsStart;
            if (seconds > range.secondsStop)
            {
                self.range.secondsStop = self.range.secondsStop - range.secondsStop;
            }
        }
    }
    else if (range.type == HBTitleSelectionRangeTypeFrames)
    {
        self.range.type = HBRangeTypeFrames;
        self.range.frameStart = range.frameStart < self.range.frameStop ? range.frameStart : self.range.frameStart;
        if (range.trim == HBTitleSelectionTrimTypeNormal)
        {
            if (range.frameStop > self.range.frameStart)
            {
                self.range.frameStop = range.frameStop;
            }
        }
        else if (range.trim == HBTitleSelectionTrimTypeEnd)
        {
            int frames = self.range.frameStop - self.range.frameStart;
            if (frames > range.frameStop)
            {
                self.range.frameStop = self.range.frameStop - range.frameStop;
            }
        }
    }
}

@end
