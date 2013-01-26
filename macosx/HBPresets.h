/*  HBPresets.h $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>


@interface HBPresets : NSObject {}

/* Called by -addFactoryPresets in Controller.mm */
- (NSMutableArray *) generateBuiltinPresets: (NSMutableArray *) UserPresets;

/* Built-In Preset Dictionaries (one for each built in preset) */
- (NSDictionary *)createDevicesPresetFolder;
- (NSDictionary *)createRegularPresetFolder;

- (NSDictionary *)createiPadPreset;
- (NSDictionary *)createAppleTV2Preset;
- (NSDictionary *)createAppleTVPreset;
- (NSDictionary *)createAppleTV3Preset;
- (NSDictionary *)createUniversalPreset;
- (NSDictionary *)createiPhoneiPodtouchPreset;
- (NSDictionary *)createiPodPreset;
- (NSDictionary *)createNormalPreset;
- (NSDictionary *)createHighProfilePreset;
- (NSDictionary *)createAndroidPreset;
- (NSDictionary *)createAndroidTabletPreset;

@end
