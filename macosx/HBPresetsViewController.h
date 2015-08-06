/*  HBPresetsViewController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBPresetsManager;
@class HBPreset;

@protocol HBPresetsViewControllerDelegate <NSObject>

- (void)selectionDidChange;
- (void)showAddPresetPanel:(id)sender;

@end

@interface HBPresetsViewController : NSViewController <NSUserInterfaceValidations>

- (instancetype)initWithPresetManager:(HBPresetsManager *)presetManager;

@property (nonatomic, readwrite, assign) id<HBPresetsViewControllerDelegate> delegate;

- (IBAction)exportPreset:(id)sender;
- (IBAction)importPreset:(id)sender;

- (void)deselect;
- (void)setSelection:(HBPreset *)preset;

- (IBAction)insertFolder:(id)sender;

@property (nonatomic, readonly, weak) HBPreset *selectedPreset;
@property (nonatomic, readonly) NSUInteger indexOfSelectedItem;

@property (nonatomic, readwrite, getter=isEnabled) BOOL enabled;

@end
