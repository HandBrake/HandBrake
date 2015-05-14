/*  HBAudioSettings.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioDefaults.h"
#import "HBAudioTrackPreset.h"
#import "HBCodingUtilities.h"
#import "hb.h"
#import "lang.h"

@interface HBAudioDefaults ()

@property (nonatomic, readwrite) int container;

@end

@implementation HBAudioDefaults

- (instancetype)init
{
    self = [super init];
    if (self) {
        _encoderFallback = HB_ACODEC_AC3;
        _trackSelectionLanguages = [[NSMutableArray alloc] init];
        _tracksArray = [[NSMutableArray alloc] init];
        _trackSelectionBehavior = HBAudioTrackSelectionBehaviorFirst;
        _container = HB_MUX_MKV;
    }
    return self;
}

- (void)addTrack
{
    HBAudioTrackPreset *track = [[HBAudioTrackPreset alloc] initWithContainer:self.container];
    [self insertObject:track inTracksArrayAtIndex:[self countOfTracksArray]];
}

- (NSArray *)audioEncoderFallbacks
{
    NSMutableArray *fallbacks = [[NSMutableArray alloc] init];
    for (const hb_encoder_t *audio_encoder = hb_audio_encoder_get_next(NULL);
         audio_encoder != NULL;
         audio_encoder  = hb_audio_encoder_get_next(audio_encoder))
    {
        if ((audio_encoder->codec  & HB_ACODEC_PASS_FLAG) == 0 &&
            (audio_encoder->muxers & self.container))
        {
            [fallbacks addObject:@(audio_encoder->name)];
        }
    }
    return fallbacks;
}

- (NSString *)isoCodeForNativeLang:(NSString *)language
{
    const iso639_lang_t *lang = lang_get_next(NULL);
    for (lang = lang_get_next(lang); lang != NULL; lang = lang_get_next(lang))
    {
        NSString *nativeLanguage = strlen(lang->native_name) ? @(lang->native_name) : @(lang->eng_name);

        if ([language isEqualToString:nativeLanguage])
        {
            return @(lang->iso639_2);
        }
    }

    return nil;
}

- (void)applyPreset:(NSDictionary *)preset
{
    // Track selection behavior
    if ([preset[@"AudioTrackSelectionBehavior"] isEqualToString:@"first"])
    {
        self.trackSelectionBehavior = HBAudioTrackSelectionBehaviorFirst;
    }
    else if ([preset[@"AudioTrackSelectionBehavior"] isEqualToString:@"all"])
    {
        self.trackSelectionBehavior = HBAudioTrackSelectionBehaviorAll;
    }
    else if ([preset[@"AudioTrackSelectionBehavior"] isEqualToString:@"none"])
    {
        self.trackSelectionBehavior = HBAudioTrackSelectionBehaviorNone;
    }
    else
    {
        // Keep the previous behavior for the old presets
        self.trackSelectionBehavior = HBAudioTrackSelectionBehaviorFirst;
    }
    self.trackSelectionLanguages = [NSMutableArray arrayWithArray:preset[@"AudioLanguageList"]];

    // If the preset is one of the built in, set some additional options
    if ([preset[@"Type"] intValue] == 0)
    {
        self.trackSelectionBehavior = HBAudioTrackSelectionBehaviorFirst;
        if (self.trackSelectionLanguages.count == 0)
        {
            if ([[NSUserDefaults standardUserDefaults] stringForKey:@"DefaultLanguage"])
            {
                NSString *lang = [self isoCodeForNativeLang:[[NSUserDefaults standardUserDefaults] stringForKey:@"DefaultLanguage"]];
                if (lang)
                {
                    [self.trackSelectionLanguages addObject:lang];
                }
            }
            if ([[NSUserDefaults standardUserDefaults] stringForKey:@"AlternateLanguage"])
            {
                NSString *lang = [self isoCodeForNativeLang:[[NSUserDefaults standardUserDefaults] stringForKey:@"AlternateLanguage"]];
                if (lang)
                {
                    [self.trackSelectionLanguages addObject:lang];
                }
            }
        }
    }

    // Passthru settings
    for (NSString *copyMask in preset[@"AudioCopyMask"])
    {
        if ([copyMask isEqualToString:@"copy:aac"])
        {
            self.allowAACPassthru = YES;
        }
        else if ([copyMask isEqualToString:@"copy:ac3"])
        {
            self.allowAC3Passthru = YES;
        }
        else if ([copyMask isEqualToString:@"copy:eac3"])
        {
            self.allowEAC3Passthru = YES;
        }
        else if ([copyMask isEqualToString:@"copy:dtshd"])
        {
            self.allowDTSHDPassthru = YES;
        }
        else if ([copyMask isEqualToString:@"copy:dts"])
        {
            self.allowDTSPassthru = YES;
        }
        else if ([copyMask isEqualToString:@"copy:mp3"])
        {
            self.allowMP3Passthru = YES;
        }
        else if ([copyMask isEqualToString:@"copy:truehd"])
        {
            self.allowTrueHDPassthru = YES;
        }
        else if ([copyMask isEqualToString:@"copy:flac"])
        {
            self.allowFLACPassthru = YES;
        }
    }

    self.secondaryEncoderMode = [preset[@"AudioSecondaryEncoderMode"] boolValue];

    if (preset[@"AudioEncoderFallback"])
    {
        // map legacy encoder names via libhb
        const char *strValue = hb_audio_encoder_sanitize_name([preset[@"AudioEncoderFallback"] UTF8String]);
        self.encoderFallback = hb_audio_encoder_get_from_name(strValue);
    }

    [self.tracksArray removeAllObjects];

    for (NSDictionary *track in preset[@"AudioList"])
    {
        HBAudioTrackPreset *newTrack = [[HBAudioTrackPreset alloc] init];
        if ([track[@"AudioEncoder"] isKindOfClass:[NSString class]])
        {
            newTrack.encoder = hb_audio_encoder_get_from_name([track[@"AudioEncoder"] UTF8String]);
        }
        if ([track[@"AudioMixdown"] isKindOfClass:[NSString class]])
        {
            newTrack.mixdown = hb_mixdown_get_from_name([track[@"AudioMixdown"] UTF8String]);
        }
        if ([track[@"AudioSamplerate"] isKindOfClass:[NSString class]])
        {
            newTrack.sampleRate = hb_audio_samplerate_get_from_name([track[@"AudioSamplerate"] UTF8String]);

            // Set to "Auto" if we didn't find a valid sample rate.
            if (newTrack.sampleRate == -1)
            {
                newTrack.sampleRate = 0;
            }
        }
        newTrack.bitRate = [track[@"AudioBitrate"] intValue];

        newTrack.drc = [track[@"AudioTrackDRCSlider"] doubleValue];
        newTrack.gain = [track[@"AudioTrackGainSlider"] doubleValue];
        [self.tracksArray addObject:newTrack];
    }
}

- (void)writeToPreset:(NSMutableDictionary *)preset
{
    // Track selection behavior
    if (self.trackSelectionBehavior == HBAudioTrackSelectionBehaviorFirst)
    {
        preset[@"AudioTrackSelectionBehavior"] = @"first";
    }
    else if (self.trackSelectionBehavior == HBAudioTrackSelectionBehaviorAll)
    {
        preset[@"AudioTrackSelectionBehavior"] = @"all";
    }
    else
    {
        preset[@"AudioTrackSelectionBehavior"] = @"none";
    }
    preset[@"AudioLanguageList"] = [self.trackSelectionLanguages copy];

    // Passthru settings
    NSMutableArray *copyMask = [NSMutableArray array];
    if (self.allowAACPassthru)
    {
        [copyMask addObject:@"copy:aac"];
    }
    if (self.allowAC3Passthru)
    {
        [copyMask addObject:@"copy:ac3"];
    }
    if (self.allowEAC3Passthru)
    {
        [copyMask addObject:@"copy:eac3"];
    }
    if (self.allowDTSHDPassthru)
    {
        [copyMask addObject:@"copy:dtshd"];
    }
    if (self.allowDTSPassthru)
    {
        [copyMask addObject:@"copy:dts"];
    }
    if (self.allowMP3Passthru)
    {
        [copyMask addObject:@"copy:mp3"];
    }
    if (self.allowTrueHDPassthru)
    {
        [copyMask addObject:@"copy:truehd"];
    }
    if (self.allowFLACPassthru)
    {
        [copyMask addObject:@"copy:flac"];
    }
    preset[@"AudioCopyMask"] = [copyMask copy];

    preset[@"AudioEncoderFallback"] = @(hb_audio_encoder_get_short_name(self.encoderFallback));

    preset[@"AudioSecondaryEncoderMode"] = @(self.secondaryEncoderMode);

    NSMutableArray *audioList = [[NSMutableArray alloc] init];

    for (HBAudioTrackPreset *track in self.tracksArray)
    {
        NSString *sampleRate = @"auto";
        if (hb_audio_samplerate_get_name(track.sampleRate))
        {
            sampleRate = @(hb_audio_samplerate_get_name(track.sampleRate));
        }
        NSDictionary *newTrack = @{@"AudioEncoder": @(hb_audio_encoder_get_short_name(track.encoder)),
                                   @"AudioMixdown": @(hb_mixdown_get_short_name(track.mixdown)),
                                   @"AudioSamplerate": sampleRate,
                                   @"AudioBitrate": @(track.bitRate),
                                   @"AudioTrackDRCSlider": @(track.drc),
                                   @"AudioTrackGainSlider": @(track.gain)};

        [audioList addObject:newTrack];
    }

    preset[@"AudioList"] = audioList;
}

- (void)validateEncoderFallbackForVideoContainer:(int)container
{
    BOOL isValid = NO;
    for (const hb_encoder_t *audio_encoder = hb_audio_encoder_get_next(NULL);
         audio_encoder != NULL;
         audio_encoder  = hb_audio_encoder_get_next(audio_encoder))
    {
        if (audio_encoder->muxers & container)
        {
            if (audio_encoder->codec == self.encoderFallback)
            {
                isValid = YES;
                break;
            }
        }
    }

    if (!isValid)
    {
        self.encoderFallback = HB_ACODEC_AC3;
    }

    self.container = container;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBAudioDefaults *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_trackSelectionBehavior = _trackSelectionBehavior;
        copy->_trackSelectionLanguages = [_trackSelectionLanguages mutableCopy];

        copy->_tracksArray = [[NSMutableArray alloc] initWithArray:_tracksArray copyItems:YES];

        copy->_allowAACPassthru = _allowAACPassthru;
        copy->_allowAC3Passthru = _allowAC3Passthru;
        copy->_allowEAC3Passthru = _allowEAC3Passthru;
        copy->_allowDTSHDPassthru = _allowDTSHDPassthru;
        copy->_allowDTSPassthru = _allowDTSPassthru;
        copy->_allowMP3Passthru = _allowMP3Passthru;
        copy->_allowTrueHDPassthru = _allowTrueHDPassthru;
        copy->_allowFLACPassthru = _allowFLACPassthru;

        copy->_encoderFallback = _encoderFallback;
        copy->_secondaryEncoderMode = _secondaryEncoderMode;
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
    [coder encodeInt:1 forKey:@"HBAudioDefaultsVersion"];

    encodeInteger(_trackSelectionBehavior);
    encodeObject(_trackSelectionLanguages);

    encodeObject(_tracksArray);

    encodeBool(_allowAACPassthru);
    encodeBool(_allowAC3Passthru);
    encodeBool(_allowEAC3Passthru);
    encodeBool(_allowDTSHDPassthru);
    encodeBool(_allowDTSPassthru);
    encodeBool(_allowMP3Passthru);
    encodeBool(_allowTrueHDPassthru);
    encodeBool(_allowFLACPassthru);

    encodeInt(_encoderFallback);
    encodeBool(_secondaryEncoderMode);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInteger(_trackSelectionBehavior);
    decodeObject(_trackSelectionLanguages, NSMutableArray);

    decodeObject(_tracksArray, NSMutableArray);

    decodeBool(_allowAACPassthru);
    decodeBool(_allowAC3Passthru);
    decodeBool(_allowEAC3Passthru);
    decodeBool(_allowDTSHDPassthru);
    decodeBool(_allowDTSPassthru);
    decodeBool(_allowMP3Passthru);
    decodeBool(_allowTrueHDPassthru);
    decodeBool(_allowFLACPassthru);

    decodeInt(_encoderFallback);
    decodeBool(_secondaryEncoderMode);

    return self;
}

#pragma mark KVC

- (NSUInteger)countOfTracksArray
{
    return self.tracksArray.count;
}

- (HBAudioTrackPreset *)objectInTracksArrayAtIndex:(NSUInteger)index
{
    return self.tracksArray[index];
}

- (void)insertObject:(HBAudioTrackPreset *)track inTracksArrayAtIndex:(NSUInteger)index;
{
    [self.tracksArray insertObject:track atIndex:index];
}

- (void)removeObjectFromTracksArrayAtIndex:(NSUInteger)index
{
    [self.tracksArray removeObjectAtIndex:index];
}

@end
