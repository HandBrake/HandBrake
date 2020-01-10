/*  HBJob+HBJob_Private.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <HandBrakeKit/HandBrakeKit.h>

NS_ASSUME_NONNULL_BEGIN

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

NS_ASSUME_NONNULL_END

