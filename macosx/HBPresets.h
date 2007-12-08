/*  HBPresets.h $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>


@interface HBPresets : NSObject {

}
/* Called by -addFactoryPresets in Controller.mm */
- (NSMutableArray *) generateBuiltinPresets: (NSMutableArray *) UserPresets;

/* Built-In Preset Dictionaries (one for each built in preset) */
- (NSDictionary *)createIpodLowPreset;
- (NSDictionary *)createIpodHighPreset;
- (NSDictionary *)createAppleTVPreset;
- (NSDictionary *)createPSThreePreset;  
- (NSDictionary *)createPSPPreset;
- (NSDictionary *)createNormalPreset;
- (NSDictionary *)createClassicPreset;
- (NSDictionary *)createQuickTimePreset;
- (NSDictionary *)createFilmPreset;
- (NSDictionary *)createTelevisionPreset;
- (NSDictionary *)createAnimationPreset;
- (NSDictionary *)createBedlamPreset;
- (NSDictionary *)createiPhonePreset;
- (NSDictionary *)createDeuxSixQuatrePreset;
- (NSDictionary *)createBrokePreset;
- (NSDictionary *)createBlindPreset;
- (NSDictionary *)createCRFPreset;
- (NSDictionary *)create360Preset;

@end
