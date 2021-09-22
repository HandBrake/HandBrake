/*  HBTitlePrivate.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBTitle.h"
#import "HBPreset.h"
#include "handbrake/handbrake.h"

NS_ASSUME_NONNULL_BEGIN

@interface HBTitle (Private)

/**
 *  Returns an HBTitle object initialized with a given title.
 *  It must be called only inside HBCore.
 *
 *  @param title    the libhb title to wrap.
 *  @param featured whether the title is the featured one or not.
 */
- (instancetype)initWithTitle:(hb_title_t *)title handle:(hb_handle_t *)handle featured:(BOOL)featured;

/**
 *  The internal libhb structure.
 */
@property (nonatomic, readonly) hb_title_t *hb_title;

- (nullable NSDictionary *)jobSettingsWithPreset:(HBPreset *)preset;

@end

NS_ASSUME_NONNULL_END
