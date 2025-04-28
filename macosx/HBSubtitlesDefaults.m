/*  HBSubtitlesSettings.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesDefaults.h"
#import "HBCodingUtilities.h"
#import "HBMutablePreset.h"

@implementation HBSubtitlesDefaults

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _trackSelectionLanguages = [[NSMutableArray alloc] init];
        _passthruName = NO;
    }
    return self;
}

#pragma mark - Properties

- (void)setTrackSelectionBehavior:(HBSubtitleTrackSelectionBehavior)trackSelectionBehavior
{
    if (trackSelectionBehavior != _trackSelectionBehavior)
    {
        [[self.undo prepareWithInvocationTarget:self] setTrackSelectionBehavior:_trackSelectionBehavior];
    }
    _trackSelectionBehavior = trackSelectionBehavior;
}

- (void)setAddForeignAudioSearch:(BOOL)addForeignAudioSearch
{
    if (addForeignAudioSearch != _addForeignAudioSearch)
    {
        [[self.undo prepareWithInvocationTarget:self] setAddForeignAudioSearch:_addForeignAudioSearch];
    }
    _addForeignAudioSearch = addForeignAudioSearch;
}

- (void)setAddForeignAudioSubtitle:(BOOL)addForeignAudioSubtitle
{
    if (addForeignAudioSubtitle != _addForeignAudioSubtitle)
    {
        [[self.undo prepareWithInvocationTarget:self] setAddForeignAudioSubtitle:_addForeignAudioSubtitle];
    }
    _addForeignAudioSubtitle = addForeignAudioSubtitle;
}

- (void)setAddCC:(BOOL)addCC
{
    if (addCC != _addCC)
    {
        [[self.undo prepareWithInvocationTarget:self] setAddCC:_addCC];
    }
    _addCC = addCC;
}

- (void)setBurnInBehavior:(HBSubtitleTrackBurnInBehavior)burnInBehavior
{
    if (burnInBehavior != _burnInBehavior)
    {
        [[self.undo prepareWithInvocationTarget:self] setBurnInBehavior:_burnInBehavior];
    }
    _burnInBehavior = burnInBehavior;
}

- (void)setBurnInDVDSubtitles:(BOOL)burnInDVDSubtitles
{
    if (burnInDVDSubtitles != _burnInDVDSubtitles)
    {
        [[self.undo prepareWithInvocationTarget:self] setBurnInDVDSubtitles:_burnInDVDSubtitles];
    }
    _burnInDVDSubtitles = burnInDVDSubtitles;
}

- (void)setBurnInBluraySubtitles:(BOOL)burnInBluraySubtitles
{
    if (burnInBluraySubtitles != _burnInBluraySubtitles)
    {
        [[self.undo prepareWithInvocationTarget:self] setBurnInBluraySubtitles:_burnInBluraySubtitles];
    }
    _burnInBluraySubtitles = burnInBluraySubtitles;
}

- (void)setPassthruName:(BOOL)passthruName
{
    if (passthruName != _passthruName)
    {
        [[self.undo prepareWithInvocationTarget:self] setPassthruName:_passthruName];
    }
    _passthruName = passthruName;
}

#pragma mark - HBPresetCoding

- (BOOL)applyPreset:(HBPreset *)preset error:(NSError * __autoreleasing *)outError
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

    NSString *burnInBehavior = preset[@"SubtitleBurnBehavior"];

    if ([burnInBehavior isEqualToString:@"foreign"])
    {
        self.burnInBehavior = HBSubtitleTrackBurnInBehaviorForeignAudio;
    }
    else if ([burnInBehavior isEqualToString:@"first"])
    {
        self.burnInBehavior = HBSubtitleTrackBurnInBehaviorFirst;
    }
    else if ([burnInBehavior isEqualToString:@"foreign_first"])
    {
        self.burnInBehavior = HBSubtitleTrackBurnInBehaviorForeignAudioThenFirst;
    }
    else
    {
        self.burnInBehavior = HBSubtitleTrackBurnInBehaviorNone;
    }

    self.burnInDVDSubtitles = [preset[@"SubtitleBurnDVDSub"] boolValue];
    self.burnInBluraySubtitles = [preset[@"SubtitleBurnBDSub"] boolValue];

    self.passthruName = [preset[@"SubtitleTrackNamePassthru"] boolValue];

    return YES;
}

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    [self applyPreset:preset error:NULL];
}

- (void)writeToPreset:(HBMutablePreset *)preset
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

    preset[@"SubtitleLanguageList"] = [self.trackSelectionLanguages copy];
    preset[@"SubtitleAddCC"] = @(self.addCC);
    preset[@"SubtitleAddForeignAudioSearch"] = @(self.addForeignAudioSearch);
    preset[@"SubtitleAddForeignAudioSubtitle"] = @(self.addForeignAudioSubtitle);

    if (self.burnInBehavior == HBSubtitleTrackBurnInBehaviorForeignAudio)
    {
        preset[@"SubtitleBurnBehavior"] = @"foreign";
    }
    else if (self.burnInBehavior == HBSubtitleTrackBurnInBehaviorFirst)
    {
        preset[@"SubtitleBurnBehavior"] = @"first";
    }
    else if (self.burnInBehavior == HBSubtitleTrackBurnInBehaviorForeignAudioThenFirst)
    {
        preset[@"SubtitleBurnBehavior"] = @"foreign_first";
    }
    else
    {
        preset[@"SubtitleBurnBehavior"] = @"none";
    }

    preset[@"SubtitleBurnDVDSub"] = @(self.burnInDVDSubtitles);
    preset[@"SubtitleBurnBDSub"] = @(self.burnInBluraySubtitles);

    preset[@"SubtitleTrackNamePassthru"] = @(self.passthruName);

}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBSubtitlesDefaults *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_trackSelectionBehavior = _trackSelectionBehavior;
        copy->_trackSelectionLanguages = [_trackSelectionLanguages mutableCopy];

        copy->_addForeignAudioSearch = _addForeignAudioSearch;
        copy->_addForeignAudioSubtitle = _addForeignAudioSubtitle;
        copy->_addCC = _addCC;

        copy->_burnInBehavior = _burnInBehavior;
        copy->_burnInDVDSubtitles = _burnInDVDSubtitles;
        copy->_burnInBluraySubtitles = _burnInBluraySubtitles;

        copy->_passthruName = _passthruName;
    }

    return copy;
}

#pragma mark - NSCoding

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBSubtitlesDefaultsVersion"];

    encodeInteger(_trackSelectionBehavior);
    encodeObject(_trackSelectionLanguages);

    encodeBool(_addForeignAudioSearch);
    encodeBool(_addForeignAudioSubtitle);
    encodeBool(_addCC);

    encodeInteger(_burnInBehavior);
    encodeBool(_burnInDVDSubtitles);
    encodeBool(_burnInBluraySubtitles);

    encodeBool(_passthruName);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInteger(_trackSelectionBehavior);
    if (_trackSelectionBehavior < HBSubtitleTrackSelectionBehaviorNone || _trackSelectionBehavior > HBSubtitleTrackSelectionBehaviorAll)
    {
        goto fail;
    }
    decodeCollectionOfObjectsOrFail(_trackSelectionLanguages, NSMutableArray, NSString);

    decodeBool(_addForeignAudioSearch);
    decodeBool(_addForeignAudioSubtitle);
    decodeBool(_addCC);

    decodeInteger(_burnInBehavior);
    if (_burnInBehavior < HBSubtitleTrackBurnInBehaviorNone || _burnInBehavior > HBSubtitleTrackBurnInBehaviorForeignAudioThenFirst)
    {
        goto fail;
    }
    decodeBool(_burnInDVDSubtitles);
    decodeBool(_burnInBluraySubtitles);

    decodeBool(_passthruName);

    return self;

fail:
    return nil;
}


@end
