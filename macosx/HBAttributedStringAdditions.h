//
//  HBAttributedStringAdditions.h
//  HandBrake
//
//  Created by Damiano Galassi on 16/01/15.
//
//

#import <Foundation/Foundation.h>

@interface NSMutableAttributedString (HBAttributedStringAdditions)

- (void)appendString:(NSString *)aString withAttributes:(NSDictionary *)aDictionary;

@end

