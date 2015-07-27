/* $Id: HBPreviewController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBPreviewGenerator;
@class HBPicture;

@interface HBPreviewController : NSWindowController <NSWindowDelegate>

@property (nonatomic, strong) HBPreviewGenerator *generator;
@property (nonatomic, strong) HBPicture *picture;

@end
