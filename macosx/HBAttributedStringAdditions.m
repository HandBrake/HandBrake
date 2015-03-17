//
//  HBAttributedStringAdditions.m
//  HandBrake
//
//  Created by Damiano Galassi on 16/01/15.
//
//

#import "HBAttributedStringAdditions.h"

@implementation NSMutableAttributedString (HBAttributedStringAdditions)

- (void)appendString:(NSString *)aString withAttributes:(NSDictionary *)aDictionary
{
    NSAttributedString *s = [[NSAttributedString alloc] initWithString:aString
                                                             attributes:aDictionary];
    [self appendAttributedString:s];
}

@end