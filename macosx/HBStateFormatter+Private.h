//
//  HBStateFormatter+Private.h
//  HandBrake
//
//  Created by Damiano Galassi on 24/02/16.
//
//

#import <Foundation/Foundation.h>
#import "HBStateFormatter.h"
#include "handbrake/handbrake.h"

NS_ASSUME_NONNULL_BEGIN

@interface HBStateFormatter (Private)

/**
 *  Returns a string containing the formatted value of the provided hb_state_t struct.
 *
 *  @param s     hb_state_t
 *  @param title the title of the current job
 */
- (NSString *)stateToString:(hb_state_t)s;

/**
 *  Returns a float containing the completion percent.
 *  the float range is [0,1]
 *
 *  @param s hb_state_t
 */
- (float)stateToPercentComplete:(hb_state_t)s;

@end

NS_ASSUME_NONNULL_END
