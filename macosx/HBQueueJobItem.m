/*  HBQueueJobItem.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueJobItem.h"

#import "HBCodingUtilities.h"
#import "HBAttributedStringAdditions.h"

static NSDateFormatter *_dateFormatter = nil;
static NSNumberFormatter *_numberFormatter = nil;
static NSByteCountFormatter *_byteFormatter = nil;

static NSDictionary     *detailAttr;
static NSDictionary     *detailBoldAttr;
static NSDictionary     *shortHeightAttr;

@interface HBQueueJobItem ()

@property (nonatomic) NSTimeInterval encodeDuration;
@property (nonatomic) NSTimeInterval pauseDuration;

@property (nonatomic, nullable) NSDate *pausedDate;
@property (nonatomic, nullable) NSDate *resumedDate;

@property (nonatomic) double avgFps;
@property (nonatomic) NSUInteger fileSize;
@property (nonatomic) NSUInteger sourceFileSize;

@property (nonatomic, readwrite, nullable) NSAttributedString *attributedStatistics;

@end

@implementation HBQueueJobItem

+ (void)initialize
{
    if (self == [HBQueueJobItem class]) {
        _dateFormatter = [[NSDateFormatter alloc] init];
        _dateFormatter.dateStyle = NSDateFormatterLongStyle;
        _dateFormatter.timeStyle = NSDateFormatterLongStyle;

        _numberFormatter = [[NSNumberFormatter alloc] init];
        _numberFormatter.numberStyle = NSNumberFormatterDecimalStyle;
        _numberFormatter.maximumFractionDigits = 2;

        _byteFormatter = [[NSByteCountFormatter alloc] init];

        CGFloat indent = 100;
        NSBundle *bundle = [NSBundle bundleForClass:[HBQueueJobItem class]];
        NSString *currentLocalization = bundle.preferredLocalizations.firstObject;
        if ([currentLocalization hasPrefix:@"de"])
        {
            indent = 120;
        }
        else if ([currentLocalization hasPrefix:@"fr"] || [currentLocalization hasPrefix:@"pt"])
        {
            indent = 114;
        }

        // Attributes
        NSMutableParagraphStyle *ps = [NSParagraphStyle.defaultParagraphStyle mutableCopy];
        ps.headIndent = indent;
        ps.paragraphSpacing = 1.0;
        ps.tabStops = @[[[NSTextTab alloc] initWithType:NSRightTabStopType location:indent - 2],
                        [[NSTextTab alloc] initWithType:NSLeftTabStopType location:indent]];

        detailAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:NSFont.smallSystemFontSize],
                       NSParagraphStyleAttributeName: ps,
                       NSForegroundColorAttributeName: NSColor.labelColor};

        detailBoldAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:NSFont.smallSystemFontSize],
                           NSParagraphStyleAttributeName: ps,
                           NSForegroundColorAttributeName: NSColor.labelColor};

        shortHeightAttr = @{NSFontAttributeName: [NSFont systemFontOfSize:6.0]};
    }
}

@synthesize job = _job;
@synthesize attributedDescription = _attributedDescription;

- (instancetype)initWithJob:(HBJob *)job
{
    self = [super init];
    if (self)
    {
        _job = job;
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
        self.activityLogURL = nil;
    }
}

- (NSString *)title
{
    return _job.destinationFileName;
}

- (BOOL)hasFileRepresentation
{
    return YES;
}

- (NSImage *)image
{
    return [NSImage imageNamed:@"JobSmall"];;
}

- (NSURL *)fileURL
{
    return _job.fileURL;
}

- (NSString *)destinationFileName
{
    return _job.destinationFileName;
}

- (NSURL *)destinationFolderURL
{
    return _job.destinationFolderURL;
}

- (NSURL *)destinationURL
{
    return _job.destinationURL;
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
    if (self.endedDate == nil && self.startedDate == nil)
    {
        return nil;
    }

    if (_attributedStatistics == nil)
    {
        NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];

        if (self.startedDate)
        {
            [attrString appendString:@"\t" withAttributes:detailAttr];
            [attrString appendString:NSLocalizedString(@"Started at:", @"Job statistics") withAttributes:detailBoldAttr];
            [attrString appendString:@" \t" withAttributes:detailAttr];
            [attrString appendString:[_dateFormatter stringFromDate:self.startedDate] withAttributes:detailAttr];
        }

        if (self.startedDate && self.endedDate)
        {
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

            if (self.avgFps > 0)
            {
                [attrString appendString:@"\n\t" withAttributes:detailAttr];
                [attrString appendString:NSLocalizedString(@"Average speed:", @"Job statistics") withAttributes:detailBoldAttr];
                [attrString appendString:@" \t" withAttributes:detailAttr];
                NSString *formattedAvgFps = [_numberFormatter stringFromNumber:@(self.avgFps)];
                [attrString appendString:[NSString stringWithFormat:NSLocalizedString(@"%@ fps", @"Job statistics"), formattedAvgFps] withAttributes:detailAttr];
            }

            [attrString appendString:@"\n\n" withAttributes:shortHeightAttr];
            [attrString appendString:@"\t" withAttributes:detailAttr];

            [attrString appendString:NSLocalizedString(@"Size:", @"Job statistics") withAttributes:detailBoldAttr];
            [attrString appendString:@" \t" withAttributes:detailAttr];
            [attrString appendString:[_byteFormatter stringFromByteCount:self.fileSize] withAttributes:detailAttr];

            if (self.job.isStream && self.sourceFileSize > 0 && self.fileSize > 0)
            {
                double difference = 100.f / self.sourceFileSize * self.fileSize;
                NSString *formattedDifference = [_numberFormatter stringFromNumber:@(difference)];
                [attrString appendString:[NSString stringWithFormat:NSLocalizedString(@" (%@ %% of the source file)", @"Job statistics"), formattedDifference] withAttributes:detailAttr];
            }
        }

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
    self.avgFps = 0;
    self.fileSize = 0;
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

- (void)setStartedDate:(NSDate *)startedDate
{
    _startedDate = startedDate;
    self.attributedStatistics = nil;
}

- (void)setEndedDate:(NSDate *)endedDate
{
    _endedDate = endedDate;
    if (endedDate && self.pausedDate)
    {
        [self resumedAtDate:endedDate];
    }
    if (endedDate && self.startedDate)
    {
        self.encodeDuration = [self.endedDate timeIntervalSinceDate:self.startedDate];
        self.encodeDuration -= self.pauseDuration;
    }

    NSDictionary<NSURLResourceKey, id> *values;

    [self.destinationURL removeCachedResourceValueForKey:NSURLFileSizeKey];
    values = [self.destinationURL resourceValuesForKeys:@[NSURLFileSizeKey] error:NULL];
    self.fileSize = [values[NSURLFileSizeKey] integerValue];

    [self.fileURL removeCachedResourceValueForKey:NSURLFileSizeKey];
    values = [self.fileURL resourceValuesForKeys:@[NSURLFileSizeKey] error:NULL];
    self.sourceFileSize = [values[NSURLFileSizeKey] integerValue];

    self.attributedStatistics = nil;
}

- (void)setDoneWithResult:(HBCoreResult)result
{
    self.avgFps = result.avgFps;

    switch (result.code) {
        case HBCoreResultCodeDone:
            self.state = HBQueueItemStateCompleted;
            break;
        case HBCoreResultCodeCanceled:
            self.state = HBQueueItemStateCanceled;
            break;
        default:
            self.state = HBQueueItemStateFailed;
            break;
    }
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
    encodeInteger(_state);
    encodeObject(_job);

    encodeObject(_activityLogURL);

    encodeDouble(_encodeDuration);
    encodeDouble(_pauseDuration);
    encodeDouble(_avgFps);

    encodeObject(_startedDate);
    encodeObject(_endedDate);

    encodeInteger(_fileSize);
    encodeInteger(_sourceFileSize);
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)decoder
{
    int version = [decoder decodeIntForKey:versionKey];

    if (version == 1 && (self = [super init]))
    {
        decodeInteger(_state); if (_state < HBQueueItemStateReady || _state > HBQueueItemStateRescanning) { goto fail; }
        decodeObjectOrFail(_job, HBJob);

        decodeObject(_activityLogURL, NSURL);

        decodeDouble(_encodeDuration);
        decodeDouble(_pauseDuration);
        decodeDouble(_avgFps);

        decodeObject(_startedDate, NSDate);
        decodeObject(_endedDate, NSDate);

        decodeInteger(_fileSize);
        decodeInteger(_sourceFileSize);

        return self;
    }
fail:
    return nil;
}

@end
