/*  HBQueueItem.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueItem.h"

#import "HBCodingUtilities.h"

@interface HBQueueItem ()

@property (nonatomic, nullable) NSDate *pausedDate;
@property (nonatomic, nullable) NSDate *resumedDate;

@end

@implementation HBQueueItem

@synthesize job = _job;
@synthesize attributedTitleDescription = _attributedTitleDescription;
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

- (NSAttributedString *)attributedTitleDescription
{
    if (_attributedTitleDescription == nil) {
        _attributedTitleDescription = _job.attributedTitleDescription;
    }
    return _attributedTitleDescription;
}

- (NSAttributedString *)attributedDescription
{
    if (_attributedDescription == nil) {
        _attributedDescription = _job.attributedDescription;
    }
    return _attributedDescription;
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
}

- (void)pausedAtDate:(NSDate *)date
{
    self.pausedDate = date;
}

- (void)resumedAtDate:(NSDate *)date
{
    self.resumedDate = date;
    self.pauseDuration += [self.resumedDate timeIntervalSinceDate:self.pausedDate];
}

- (void)setEndedDate:(NSDate *)endedDate
{
    _endedDate = endedDate;
    self.encodeDuration = [self.startedDate timeIntervalSinceDate:self.endedDate];
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
