/*  HBAttributedStringAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAttributedStringAdditions.h"

@implementation NSMutableAttributedString (HBAttributedStringAdditions)

- (void)appendString:(NSString *)aString withAttributes:(NSDictionary *)aDictionary
{
    NSAttributedString *s = [[NSAttributedString alloc] initWithString:aString
                                                             attributes:aDictionary];
    [self appendAttributedString:s];
}

@end