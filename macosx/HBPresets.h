/*  HBPresets.h $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>


@interface HBPresets : NSObject {}

/* Called by -addFactoryPresets in Controller.mm */
- (NSMutableArray *) generateBuiltinPresets: (NSMutableArray *) UserPresets;

/* Built-In Preset Dictionaries (one for each built in preset) */
- (NSDictionary *)createApplePresetFolder;
- (NSDictionary *)createBasicPresetFolder;
- (NSDictionary *)createGamingConsolesPresetFolder;
- (NSDictionary *)createHiProfilePresetFolder;

- (NSDictionary *)create360Preset;
- (NSDictionary *)createAnimationPreset;
- (NSDictionary *)createAppleTVPreset;
- (NSDictionary *)createAppleTVLegacuPreset;
- (NSDictionary *)createAppleUniversalPreset;
- (NSDictionary *)createClassicPreset;
- (NSDictionary *)createCRFPreset;
- (NSDictionary *)createFilmPreset;
- (NSDictionary *)createiPhonePreset;
- (NSDictionary *)createiPhoneLegacyPreset;
- (NSDictionary *)createIpodHighPreset;
- (NSDictionary *)createIpodLowPreset;
- (NSDictionary *)createNormalPreset;
- (NSDictionary *)createPSPPreset;
- (NSDictionary *)createPSThreePreset;  
- (NSDictionary *)createQuickTimePreset;
- (NSDictionary *)createTelevisionPreset;

@end
