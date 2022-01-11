/*  HBQueueActionItem.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueActionItem.h"
#import "HBCodingUtilities.h"

@implementation HBQueueActionStopItem

#pragma mark - NSSecureCoding

- (instancetype)init
{
    self = [super init];
    if (self) {
        _state = HBQueueItemStateReady;
    }
    return self;
}

- (NSString *)title
{
    return NSLocalizedString(@"Stop", @"Queue -> Stop action");
}

- (BOOL)hasFileRepresentation
{
    return NO;
}

- (NSURL *)destinationURL
{
    return nil;
}

- (NSAttributedString *)attributedDescription
{
    return [[NSAttributedString alloc] initWithString:NSLocalizedString(@"Stop action.", @"Queue -> Stop action")
                                           attributes:@{NSFontAttributeName: [NSFont systemFontOfSize:NSFont.smallSystemFontSize]}];
}

- (NSImage *)image
{
    return [NSImage imageNamed:@"EncodeCanceled"];
}

+ (BOOL)supportsSecureCoding
{
    return YES;
}

static NSString *versionKey = @"HBQueueActionItemVersion";

- (void)encodeWithCoder:(nonnull NSCoder *)coder {
    [coder encodeInt:1 forKey:versionKey];
    encodeInteger(_state);
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)decoder {
    int version = [decoder decodeIntForKey:versionKey];

    if (version == 1 && (self = [super init]))
    {
        decodeInteger(_state); if (_state < HBQueueItemStateReady || _state > HBQueueItemStateRescanning) { goto fail; }
        return self;
    }
fail:
    return nil;
}

@end
