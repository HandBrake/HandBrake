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
- (NSDictionary *)createLegacyPresetFolder;

- (NSDictionary *)createiPadPreset;
- (NSDictionary *)createiPhone4Preset;
- (NSDictionary *)createAppleTv2Preset;
- (NSDictionary *)createAppleTVPreset;
- (NSDictionary *)createAppleUniversalPreset;
- (NSDictionary *)createClassicPreset;
- (NSDictionary *)createiPhonePreset;
- (NSDictionary *)createIpodHighPreset;
- (NSDictionary *)createIpodLowPreset;
- (NSDictionary *)createNormalPreset;
- (NSDictionary *)createHighProfilePreset;
- (NSDictionary *)createAndroidMidPreset;
- (NSDictionary *)createAndroidHighPreset;

@end
