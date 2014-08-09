/*  HBPresetsViewController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBViewValidation.h"

@class HBPresetsManager;
@class HBPreset;

@protocol HBPresetsViewControllerDelegate <NSObject>

- (void)selectionDidChange;
- (void)showAddPresetPanel:(id)sender;

@end

@interface HBPresetsViewController : NSViewController <HBViewValidation>

- (instancetype)initWithPresetManager:(HBPresetsManager *)presetManager;

@property (nonatomic, readwrite, assign) id<HBPresetsViewControllerDelegate> delegate;

- (void)deselect;
- (void)selectPreset:(HBPreset *)preset;

- (IBAction)insertFolder:(id)sender;

@property (nonatomic, readonly) HBPreset *selectedPreset;
@property (nonatomic, readonly) NSUInteger indexOfSelectedItem;

@end
