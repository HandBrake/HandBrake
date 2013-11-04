/* $Id: HBPreviewController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#include "hb.h"

@class HBController;

@interface HBPreviewController : NSWindowController <NSWindowDelegate>

@property (nonatomic) BOOL deinterlacePreview;
@property (nonatomic, readonly) NSString *pictureSizeInfoString;

@property (nonatomic, assign) HBController *delegate;
@property (nonatomic, assign) hb_handle_t *handle;
@property (nonatomic, assign) hb_title_t *title;

- (void) reload;

@end
