/* $Id: Controller.h,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@import HandBrakeKit;

@class HBAppDelegate;
@class HBQueue;

NS_ASSUME_NONNULL_BEGIN

@interface HBController : NSWindowController

- (instancetype)initWithDelegate:(HBAppDelegate *)delegate queue:(HBQueue *)queue presetsManager:(HBPresetsManager *)manager;

- (void)launchAction;

- (void)openURLs:(NSArray<NSURL *> *)fileURLs recursive:(BOOL)recursive;
- (void)openJob:(HBJob *)job completionHandler:(void (^)(BOOL result))handler;

- (IBAction)browseSources:(id)sender;

- (IBAction)showPreviewWindow:(id)sender;

// Queue
- (IBAction)addToQueue:(id)sender;
- (IBAction)addTitlesToQueue:(id)sender;
- (IBAction)addAllTitlesToQueue:(id)sender;

- (IBAction)toggleStartCancel:(id)sender;
- (IBAction)togglePauseResume:(id)sender;
- (IBAction)togglePresets:(id)sender;
- (IBAction)selectPresetFromMenu:(id)sender;

// Manage User presets
- (IBAction)showAddPresetPanel:(id)sender;
- (IBAction)showRenamePresetPanel:(id)sender;
- (IBAction)selectDefaultPreset:(id)sender;

- (IBAction)deletePreset:(id)sender;
- (IBAction)reloadPreset:(id)sender;

@end

NS_ASSUME_NONNULL_END

