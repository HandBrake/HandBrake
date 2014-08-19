/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBFilters.h"
#include "hb.h"

@protocol HBPictureControllerDelegate <NSObject>

- (void) pictureSettingsDidChange;

@end

@interface HBPictureController : NSWindowController <NSWindowDelegate>

@property (nonatomic, readwrite, retain) HBFilters *filters;
@property (nonatomic, readwrite) BOOL autoCrop;

@property (nonatomic, readwrite, assign) id <HBPictureControllerDelegate> delegate;

- (void) setHandle:(hb_handle_t *) handle;
- (void) setTitle:(hb_title_t *) title;

- (IBAction) showPictureWindow: (id)sender;
- (IBAction) showPreviewWindow: (id)sender;

- (NSString *) pictureSizeInfoString;

@end
