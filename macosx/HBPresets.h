/*  HBPresets.h $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@interface HBPresets : NSObject {}

/* Called by -addFactoryPresets in Controller.m */
- (NSMutableArray *)generateBuiltinPresets:(NSMutableArray *)UserPresets;

/* Dictionaries for preset folders ("Devices, "Regular") */
- (NSDictionary *)createDevicesPresetFolder;
- (NSDictionary *)createRegularPresetFolder;

/* Dictionaries for individual presets ("Devices" folder) */
- (NSDictionary *)createUniversalPreset;
- (NSDictionary *)createiPodPreset;
- (NSDictionary *)createiPhoneiPodtouchPreset;
- (NSDictionary *)createiPadPreset;
- (NSDictionary *)createAppleTVPreset;
- (NSDictionary *)createAppleTV2Preset;
- (NSDictionary *)createAppleTV3Preset;
- (NSDictionary *)createAndroidPreset;
- (NSDictionary *)createAndroidTabletPreset;

/* Dictionaries for individual presets ("Regular" folder) */
- (NSDictionary *)createNormalPreset;
- (NSDictionary *)createHighProfilePreset;

@end
