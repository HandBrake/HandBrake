/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#include "hb.h"

@protocol HBPictureControllerDelegate <NSObject>

- (void) pictureSettingsDidChange;

@end

@interface HBPictureController : NSWindowController <NSWindowDelegate>

@property (nonatomic, readwrite) NSInteger detelecine;
@property (nonatomic, readwrite, copy) NSString *detelecineCustomString;

@property (nonatomic, readwrite) NSInteger deinterlace;
@property (nonatomic, readwrite, copy) NSString *deinterlaceCustomString;

@property (nonatomic, readwrite) NSInteger decomb;
@property (nonatomic, readwrite, copy) NSString *decombCustomString;

@property (nonatomic, readwrite) NSInteger denoise;
@property (nonatomic, readwrite, copy) NSString *denoiseCustomString;

@property (nonatomic, readwrite) NSInteger deblock;
@property (nonatomic, readwrite) NSInteger grayscale;

@property (nonatomic, readwrite) BOOL autoCrop;
@property (nonatomic, readwrite) NSInteger useDecomb;

@property (nonatomic, readwrite, assign) id <HBPictureControllerDelegate> delegate;

- (void) setHandle:(hb_handle_t *) handle;
- (void) setTitle:(hb_title_t *) title;

- (IBAction) showPictureWindow: (id)sender;
- (IBAction) showPreviewWindow: (id)sender;

- (NSString *) pictureSizeInfoString;

@end
