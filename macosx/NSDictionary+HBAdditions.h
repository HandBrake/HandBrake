/*  NSDictionary+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#include "handbrake/hb_dict.h"

NS_ASSUME_NONNULL_BEGIN

@interface NSDictionary (HBValueAdditions)

- (instancetype)initWithHBDict:(const hb_dict_t *)dict;
- (hb_dict_t *)hb_value;

@end

NS_ASSUME_NONNULL_END
