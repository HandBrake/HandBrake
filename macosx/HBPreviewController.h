/* $Id: HBPreviewController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBPreviewGenerator;

@protocol HBPreviewControllerDelegate <NSObject>

- (IBAction)showPicturePanel:(id)sender;

@end

@interface HBPreviewController : NSWindowController <NSWindowDelegate>

- (id)initWithDelegate:(id <HBPreviewControllerDelegate>)delegate;

@property (nonatomic, strong) HBPreviewGenerator *generator;

@end
