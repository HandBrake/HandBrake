/*  HBTitle.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#include "hb.h"

/**
 * HBTitles is an interface to the low-level hb_title_t.
 * the properties are lazy-loaded.
 */
@interface HBTitle : NSObject

/**
 *  Returns an HBTitle object initialized with a given title.
 *  It must be called only inside HBCore.
 *
 *  @param title    the lihhb title to wrap.
 *  @param featured whether the title is the featured one or not.
 */
- (instancetype)initWithTitle:(hb_title_t *)title featured:(BOOL)featured;

@property (nonatomic, readonly) NSString *name;
@property (nonatomic, readonly, getter=isFeatured) BOOL featured;

@property (nonatomic, readonly) hb_title_t *hb_title;

@property (nonatomic, readonly) int angles;
@property (nonatomic, readonly) int duration;
@property (nonatomic, readonly) int frames;

@property (nonatomic, readonly) NSArray *audioTracks;
@property (nonatomic, readonly) NSArray *subtitlesTracks;
@property (nonatomic, readonly) NSArray *chapters;

@end
