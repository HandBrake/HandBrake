/*  HBPresetsViewController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBPresetsManager;
@class HBPreset;

NS_ASSUME_NONNULL_BEGIN

@protocol HBPresetsViewControllerDelegate <NSObject>

- (void)selectionDidChange;
- (void)showAddPresetPanel:(id)sender;

@end

@interface HBPresetsViewController : NSViewController <NSUserInterfaceValidations>

- (instancetype)initWithPresetManager:(HBPresetsManager *)presetManager;

@property (nonatomic, readwrite, assign) id<HBPresetsViewControllerDelegate> delegate;

- (IBAction)setDefault:(id)sender;
- (IBAction)deletePreset:(id)sender;

- (IBAction)exportPreset:(id)sender;
- (IBAction)importPreset:(id)sender;

- (IBAction)insertCategory:(id)sender;

@property (nonatomic, readwrite) HBPreset *selectedPreset;

@property (nonatomic, readwrite, getter=isEnabled) BOOL enabled;
@property (nonatomic, readwrite) BOOL showHeader;

@end

NS_ASSUME_NONNULL_END
