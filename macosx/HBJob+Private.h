//
//  HBJob+HBJob_Private.h
//  HandBrake
//
//  Created by Damiano Galassi on 29/10/16.
//
//

#import <HandBrakeKit/HandBrakeKit.h>

@interface HBVideo (Private)

- (instancetype)initWithJob:(HBJob *)job;

@property (nonatomic, readwrite, weak) HBJob *job;

- (void)containerChanged;
- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBPicture (Private)

- (instancetype)initWithTitle:(HBTitle *)title;

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBFilters (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBAudio (Private)

- (instancetype)initWithJob:(HBJob *)job;

@property (nonatomic, readwrite, weak) HBJob *job;
@property (nonatomic, readwrite) int container;

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBSubtitles (Private)

- (instancetype)initWithJob:(HBJob *)job;

@property (nonatomic, readwrite, weak) HBJob *job;
@property (nonatomic, readwrite) int container;

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBAudioDefaults (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

@interface HBSubtitlesDefaults (Private)

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings;

@end

