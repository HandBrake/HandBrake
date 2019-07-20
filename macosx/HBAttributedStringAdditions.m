/*  HBAttributedStringAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAttributedStringAdditions.h"

static NSDictionary *_monospacedAttr = nil;
static NSDictionary *_normalMonospacedAttr = nil;

@implementation NSMutableAttributedString (HBAttributedStringAdditions)

- (void)appendString:(NSString *)aString withAttributes:(NSDictionary *)aDictionary
{
    NSAttributedString *s = [[NSAttributedString alloc] initWithString:aString
                                                             attributes:aDictionary];
    [self appendAttributedString:s];
}

@end

@implementation NSString (HBAttributedStringAdditions)

- (NSAttributedString *)HB_monospacedString
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _normalMonospacedAttr = @{NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:15 weight:NSFontWeightRegular]};
    });
    return [[NSAttributedString alloc] initWithString:self attributes:_normalMonospacedAttr];
}

- (NSAttributedString *)HB_smallMonospacedString
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _monospacedAttr = @{NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:[NSFont smallSystemFontSize] weight:NSFontWeightRegular]};
    });
    return [[NSAttributedString alloc] initWithString:self attributes:_monospacedAttr];
}

@end
