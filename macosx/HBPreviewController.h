/* $Id: HBPreviewController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBController;

@class HBCore;
@class HBJob;

@protocol HBPreviewControllerDelegate <NSObject>

- (IBAction)showPicturePanel:(id)sender;

@end

@interface HBPreviewController : NSWindowController <NSWindowDelegate>

- (id)initWithDelegate:(id <HBPreviewControllerDelegate>)delegate;

@property (nonatomic, assign) HBCore *core;
@property (nonatomic, assign) HBJob *job;

/**
 *  Reloads the preview images.
 *  Usually called after a picture setting changes.
 */
- (void)reloadPreviews;

@end
