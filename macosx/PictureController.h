/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

#import "HBFilters.h"
#import "HBPicture.h"

@protocol HBPictureControllerDelegate <NSObject>

- (IBAction)showPreviewWindow:(id)sender;

@end

@interface HBPictureController : NSWindowController <NSWindowDelegate>

@property (nonatomic, readwrite, retain) HBFilters *filters;
@property (nonatomic, readwrite, retain) HBPicture *picture;

@property (nonatomic, readwrite, assign) id <HBPictureControllerDelegate> delegate;

- (void)showPictureWindow;

@end
