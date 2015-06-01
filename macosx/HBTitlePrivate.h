//
//  HBTitlePrivate.h
//  HandBrake
//
//  Created by Damiano Galassi on 30/05/15.
//
//

#import <Foundation/Foundation.h>
#import "HBTitle.h"
#include "hb.h"

@interface HBTitle (Private)

/**
 *  Returns an HBTitle object initialized with a given title.
 *  It must be called only inside HBCore.
 *
 *  @param title    the libhb title to wrap.
 *  @param featured whether the title is the featured one or not.
 */
- (instancetype)initWithTitle:(hb_title_t *)title featured:(BOOL)featured;

/**
 *  The internal libhb structure.
 */
@property (nonatomic, readonly) hb_title_t *hb_title;

@end