/*  HBSubtitlesSettings.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesDefaults.h"

@implementation HBSubtitlesDefaults

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _trackSelectionLanguages = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)applySettingsFromPreset:(NSDictionary *)preset
{
    if ([preset[@"SubtitleTrackSelectionBehavior"] isEqualToString:@"first"])
    {
        self.trackSelectionBehavior = HBSubtitleTrackSelectionBehaviorFirst;
    }
    else if ([preset[@"SubtitleTrackSelectionBehavior"] isEqualToString:@"all"])
    {
        self.trackSelectionBehavior = HBSubtitleTrackSelectionBehaviorAll;
    }
    else
    {
        self.trackSelectionBehavior = HBSubtitleTrackSelectionBehaviorNone;
    }
    self.trackSelectionLanguages = [NSMutableArray arrayWithArray:preset[@"SubtitleLanguageList"]];
    self.addCC = [preset[@"SubtitleAddCC"] boolValue];
    self.addForeignAudioSearch = [preset[@"SubtitleAddForeignAudioSearch"] boolValue];
    self.addForeignAudioSubtitle = [preset[@"SubtitleAddForeignAudioSubtitle"] boolValue];
}

- (void)prepareSubtitlesDefaultsForPreset:(NSMutableDictionary *)preset
{
    if (self.trackSelectionBehavior == HBSubtitleTrackSelectionBehaviorFirst)
    {
        preset[@"SubtitleTrackSelectionBehavior"] = @"first";
    }
    else if (self.trackSelectionBehavior == HBSubtitleTrackSelectionBehaviorAll)
    {
        preset[@"SubtitleTrackSelectionBehavior"] = @"all";
    }
    else
    {
        preset[@"SubtitleTrackSelectionBehavior"] = @"none";
    }

    preset[@"SubtitleLanguageList"] = [[self.trackSelectionLanguages copy] autorelease];
    preset[@"SubtitleAddCC"] = @(self.addCC);
    preset[@"SubtitleAddForeignAudioSearch"] = @(self.addForeignAudioSearch);
    preset[@"SubtitleAddForeignAudioSubtitle"] = @(self.addForeignAudioSubtitle);
}

- (void)dealloc
{
    [_trackSelectionLanguages release];
    [super dealloc];
}

@end
