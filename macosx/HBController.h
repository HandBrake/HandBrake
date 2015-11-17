/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBQueueController;
@class HBPresetsManager;

@class HBJob;

@interface HBController : NSWindowController

- (instancetype)initWithQueue:(HBQueueController *)queueController presetsManager:(HBPresetsManager *)manager;

- (void)launchAction;

- (BOOL)openURL:(NSURL *)fileURL;
- (BOOL)openJob:(HBJob *)job;

- (IBAction)browseSources:(id)sender;

- (IBAction)showPreviewWindow:(id)sender;

// Queue
- (IBAction)addToQueue:(id)sender;
- (IBAction)addAllTitlesToQueue:(id)sender;

- (void)setQueueState:(NSString *)info;
- (void)setQueueInfo:(NSString *)info progress:(double)progress hidden:(BOOL)hidden;

- (IBAction)rip:(id)sender;
- (IBAction)pause:(id)sender;

- (IBAction)selectPresetFromMenu:(id)sender;

// Manage User presets
- (IBAction)showAddPresetPanel:(id)sender;
- (IBAction)selectDefaultPreset:(id)sender;

@end
