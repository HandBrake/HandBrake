/*  HBPresetsManager.h $
 
 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBPreset;

/**
 *  Posted when a preset is changed/added/deleted.
 */
extern NSString *HBPresetsChangedNotification;

/**
 *  HBPresetManager
 *  Manages the load/save of presets to an in memory tree.
 */
@interface HBPresetsManager : NSObject

/**
 *  The root of the presets tree.
 */
@property (nonatomic, readonly) HBPreset *root;

/**
 *  defaultPreset and its index path in the tree
 */
@property (nonatomic, readwrite, retain) HBPreset *defaultPreset;

/**
 *  Returs a HBPresetManager with the presets loaded at the passed URL.
 *
 *  @param url the URL of the presets file to load.
 *
 *  @return the initialized presets manager.
 */
- (instancetype)initWithURL:(NSURL *)url;

/**
 *  Checks the version number of the builtin presets.
 *
 *  @return YES if the builtin presets are out of date.
 */
- (BOOL)checkBuiltInsForUpdates;

/**
 *  Saves the presets to disk.
 */
- (BOOL)savePresets;

/**
 *  Adds a given preset to the manager.
 *
 *  @param preset the preset dict.
 */
- (void)addPresetFromDictionary:(NSDictionary *)preset;

/**
 *  Adds a given preset to the manager.
 *
 *  @param preset the preset dict.
 */
- (void)addPreset:(HBPreset *)preset;

/**
 *  Deletes the presets at the specified index path.
 *
 *  @param idx the NSIndexPath of the preset to delete.
 */
- (void)deletePresetAtIndexPath:(NSIndexPath *)idx;

/**
 *  Returns the index path of the specified object.
 *
 *  @param preset the preset.
 *
 *  @return The index path whose corresponding value is equal to the preset. Returns nil if not found.
 */
- (NSIndexPath *)indexPathOfPreset:(HBPreset *)preset;

/**
 *  Adds back the built in presets.
 */
- (void)generateBuiltInPresets;

@end
