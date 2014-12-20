/*  HBJob.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob.h"
#import "HBTitle.h"

#import "HBAudioDefaults.h"
#import "HBSubtitlesDefaults.h"

#import "HBPreset.h"

#include "lang.h"

@implementation HBJob

- (instancetype)initWithTitle:(HBTitle *)title url:(NSURL *)fileURL andPreset:(HBPreset *)preset
{
    self = [super init];
    if (self) {
        NSParameterAssert(title);
        NSParameterAssert(fileURL);
        NSParameterAssert(preset);

        _title = title;
        _fileURL = [fileURL copy];

        _audioDefaults = [[HBAudioDefaults alloc] init];
        _subtitlesDefaults = [[HBSubtitlesDefaults alloc] init];

        _video = [[HBVideo alloc] init];
        _picture = [[HBPicture alloc] initWithTitle:title];
        _filters = [[HBFilters alloc] init];

        [self applyPreset:preset];
    }

    return self;
}

- (void)applyPreset:(HBPreset *)preset
{
    [@[self.audioDefaults, self.subtitlesDefaults, self.video, self.picture, self.filters] makeObjectsPerformSelector:@selector(applySettingsFromPreset:)
                                                                                                           withObject:preset.content];
}

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
}

- (id)initWithCoder:(NSCoder *)decoder
{
    return nil;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    return nil;
}

@end
