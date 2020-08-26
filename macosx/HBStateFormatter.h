/* HBStateFormatter.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 *  Instances of HBStateFormatter format and convert a hb_state_t struct to a textual representation.
 */
@interface HBStateFormatter : NSObject

/**
 *  The title to show in the output info.
 */
@property (nonatomic, readwrite, copy, nullable) NSString *title;

/**
 *  Break the output string in two lines.
 */
@property (nonatomic, readwrite) BOOL twoLines;

/**
 *  Shows the pass number in the output string
 */
@property (nonatomic, readwrite) BOOL showPassNumber;

@end

NS_ASSUME_NONNULL_END
