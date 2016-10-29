//
//  HBJob+HBJob_Private.h
//  HandBrake
//
//  Created by Damiano Galassi on 29/10/16.
//
//

#import <HandBrakeKit/HandBrakeKit.h>

@interface HBVideo (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBPicture (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBFilters (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBAudio (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBSubtitles (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBAudioDefaults (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBSubtitlesDefaults (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

