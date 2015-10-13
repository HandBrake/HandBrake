/*  HBPresetCoding.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

#import "HBPreset.h"
#import "HBMutablePreset.h"

@protocol HBPresetCoding <NSObject>

- (void)applyPreset:(HBPreset *)preset;
- (void)writeToPreset:(HBMutablePreset *)preset;

@end