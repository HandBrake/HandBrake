/* HBStateFormatter.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#include "hb.h"

NS_ASSUME_NONNULL_BEGIN

/**
 *  Instances of HBStateFormatter format and conver a hb_state_t struct to a textual representation.
 */
@interface HBStateFormatter : NSObject

/**
 *  Returns a string containing the formatted value of the provided hb_state_t struct.
 *
 *  @param s     hb_state_t
 *  @param title the title of the current job
 */
- (NSString *)stateToString:(hb_state_t)s title:(nullable NSString *)title;

/**
 *  Returns a CGFloat containing the completion percent.
 *  the CGFloat range is [0,1]
 *
 *  @param s hb_state_t
 */
- (CGFloat)stateToPercentComplete:(hb_state_t)s;

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
