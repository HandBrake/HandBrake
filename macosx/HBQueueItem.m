/*  HBQueueItem.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueItem.h"

#import "HBCodingUtilities.h"
#import "HBAttributedStringAdditions.h"

static NSDateFormatter *_dateFormatter = nil;

static NSDictionary     *detailAttr;
static NSDictionary     *detailBoldAttr;
static NSDictionary     *shortHeightAttr;

@interface HBQueueItem ()

@property (nonatomic, nullable) NSDate *pausedDate;
@property (nonatomic, nullable) NSDate *resumedDate;

@property (nonatomic, readwrite, nullable) NSAttributedString *attributedStatistics;

@end

@implementation HBQueueItem

+ (void)initialize
{
    if (self == [HBQueueItem class]) {
        _dateFormatter = [[NSDateFormatter alloc] init];
        [_dateFormatter setDateStyle:NSDateFormatterLongStyle];
        [_dateFormatter setTimeStyle:NSDateFormatterLongStyle];

        // Attributes
        NSMutableParagraphStyle *ps = [NSParagraphStyle.defaultParagraphStyle mutableCopy];
        ps.headIndent = 88.0;
        ps.paragraphSpacing = 1.0;
        ps.tabStops = @[[[NSTextTab alloc] initWithType:NSRightTabStopType location:88],
                        [[NSTextTab alloc] initWithType:NSLeftTabStopType location:90]];

        detailAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:NSFont.smallSystemFontSize],
                       NSParagraphStyleAttributeName: ps};

        detailBoldAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:NSFont.smallSystemFontSize],
                           NSParagraphStyleAttributeName: ps};

        shortHeightAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:2.0]};
    }
}

@synthesize job = _job;
@synthesize attributedDescription = _attributedDescription;

@synthesize uuid = _uuid;

- (instancetype)initWithJob:(HBJob *)job
{
    self = [super init];
    if (self)
    {
        _job = job;
        _uuid = [NSUUID UUID].UUIDString;
    }
    return self;
}

#pragma mark - Properties

- (void)setState:(HBQueueItemState)state
{
    _state = state;
    if (state == HBQueueItemStateReady)
    {
        [self resetStatistics];
    }
}

- (NSURL *)fileURL
{
    return _job.fileURL;
}

- (NSString *)outputFileName
{
    return _job.outputFileName;
}

- (NSURL *)outputURL
{
    return _job.outputURL;
}

- (NSURL *)completeOutputURL
{
    return _job.completeOutputURL;
}

- (NSAttributedString *)attributedDescription
{
    if (_attributedDescription == nil) {
        _attributedDescription = _job.attributedDescription;
    }
    return _attributedDescription;
}

- (NSAttributedString *)attributedStatistics
{
    if (self.endedDate == nil)
    {
        return nil;
    }

    if (_attributedStatistics == nil)
    {
        NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

        [attrString appendString:@"\t" withAttributes:detailAttr];
        [attrString appendString:NSLocalizedString(@"Started at:", @"Job statistics") withAttributes:detailBoldAttr];
        [attrString appendString:@" \t" withAttributes:detailAttr];
        [attrString appendString:[_dateFormatter stringFromDate:self.startedDate] withAttributes:detailAttr];
        [attrString appendString:@"\n\t" withAttributes:detailAttr];
        [attrString appendString:NSLocalizedString(@"Ended at:", @"Job statistics") withAttributes:detailBoldAttr];
        [attrString appendString:@" \t" withAttributes:detailAttr];
        [attrString appendString:[_dateFormatter stringFromDate:self.endedDate] withAttributes:detailAttr];
        [attrString appendString:@"\n\n" withAttributes:shortHeightAttr];
        [attrString appendString:@"\t" withAttributes:detailAttr];

        [attrString appendString:NSLocalizedString(@"Run time:", @"Job statistics") withAttributes:detailBoldAttr];
        [attrString appendString:@" \t" withAttributes:detailAttr];
        uint64_t encodeDuration = (uint64_t)self.encodeDuration;
        [attrString appendString:[NSString stringWithFormat:@"%02lld:%02lld:%02lld", encodeDuration / 3600, (encodeDuration/ 60) % 60, encodeDuration % 60]  withAttributes:detailAttr];
        [attrString appendString:@"\n\t"  withAttributes:detailAttr];
        [attrString appendString:NSLocalizedString(@"Paused time:", @"Job statistics") withAttributes:detailBoldAttr];
        [attrString appendString:@" \t" withAttributes:detailAttr];
        uint64_t pauseDuration = (uint64_t)self.pauseDuration;
        [attrString appendString:[NSString stringWithFormat:@"%02lld:%02lld:%02lld", pauseDuration / 3600, (pauseDuration/ 60) % 60, pauseDuration % 60]  withAttributes:detailAttr];
        [attrString appendString:@"\n" withAttributes:detailAttr];

        _attributedStatistics = attrString;
    }

    return _attributedStatistics;
}

#pragma mark - Statistics

- (void)resetStatistics
{
    self.pausedDate = nil;
    self.resumedDate = nil;
    self.startedDate = nil;
    self.endedDate = nil;
    self.encodeDuration = 0;
    self.pauseDuration = 0;
    self.attributedStatistics = nil;
}

- (void)pausedAtDate:(NSDate *)date
{
    self.pausedDate = date;
}

- (void)resumedAtDate:(NSDate *)date
{
    self.resumedDate = date;
    self.pauseDuration += [self.resumedDate timeIntervalSinceDate:self.pausedDate];
    self.pausedDate = nil;
}

- (void)setEndedDate:(NSDate *)endedDate
{
    _endedDate = endedDate;
    if (endedDate && self.pausedDate)
    {
        [self resumedAtDate:endedDate];
    }
    self.encodeDuration = [self.endedDate timeIntervalSinceDate:self.startedDate];
    self.encodeDuration -= self.pauseDuration;
}

#pragma mark - NSSecureCoding

+ (BOOL)supportsSecureCoding
{
    return YES;
}

static NSString *versionKey = @"HBQueueItemVersion";

- (void)encodeWithCoder:(nonnull NSCoder *)coder
{
    [coder encodeInt:1 forKey:versionKey];
    encodeInt(_state);
    encodeObject(_job);
    encodeObject(_uuid);

    encodeDouble(_encodeDuration);
    encodeDouble(_pauseDuration);

    encodeObject(_startedDate);
    encodeObject(_endedDate);
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)decoder
{
    int version = [decoder decodeIntForKey:versionKey];

    if (version == 1 && (self = [super init]))
    {
        decodeInt(_state); if (_state < HBQueueItemStateReady || _state > HBQueueItemStateFailed) { goto fail; }
        decodeObjectOrFail(_job, HBJob);
        decodeObjectOrFail(_uuid, NSString);

        decodeDouble(_encodeDuration);
        decodeDouble(_pauseDuration);

        decodeObject(_startedDate, NSDate);
        decodeObject(_endedDate, NSDate);

        return self;
    }
fail:
    return nil;
}

@end
