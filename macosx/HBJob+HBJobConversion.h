/*  HBJob+HBJobConversion.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBJob.h"
#include "handbrake/handbrake.h"

@interface HBJob (HBJobConversion)

/**
 *  Returns a job settings dict by applying
 *  the current preset to the job.
 */
@property (nonatomic, readonly) NSDictionary *jobDict;

/**
 *  Converts the job to a hb_job_t struct.
 */
@property (nonatomic, readonly) hb_job_t *hb_job;

@end
