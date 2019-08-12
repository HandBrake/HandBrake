/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBPicture;

NS_ASSUME_NONNULL_BEGIN

@interface HBPictureController : NSWindowController <NSWindowDelegate>

@property (nonatomic, readwrite, strong) HBPicture *picture;
@property (nonatomic, readwrite, assign) NSWindowController *previewController;

@end

NS_ASSUME_NONNULL_END

