/*  HBAttributedStringAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSString (HBAttributedStringAdditions)

- (NSAttributedString *)HB_monospacedString;
- (NSAttributedString *)HB_smallMonospacedString;

@end

@interface NSMutableAttributedString (HBAttributedStringAdditions)

- (void)appendString:(NSString *)aString withAttributes:(NSDictionary *)aDictionary;

@end

NS_ASSUME_NONNULL_END
