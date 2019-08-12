/* $Id: HBPreviewController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBPreviewGenerator;
@class HBPicture;
@class HBController;

NS_ASSUME_NONNULL_BEGIN

@interface HBPreviewController : NSWindowController <NSWindowDelegate>

@property (nonatomic, strong, nullable) HBPreviewGenerator *generator;
@property (nonatomic, strong, nullable) HBPicture *picture;

@property (nonatomic, assign) HBController *documentController;

@end

NS_ASSUME_NONNULL_END
