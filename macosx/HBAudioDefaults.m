/*  HBAudioSettings.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioDefaults.h"
#import "HBAudioTrackPreset.h"
#import "NSCodingMacro.h"
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

- (void)dealloc
{
    [_trackSelectionLanguages release];
    [_tracksArray release];
    [super dealloc];
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
    return [fallbacks autorelease];
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

- (void)applySettingsFromPreset:(NSDictionary *)preset
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
    self.allowAACPassthru = [preset[@"AudioAllowAACPass"] boolValue];
    self.allowAC3Passthru = [preset[@"AudioAllowAC3Pass"] boolValue];
    self.allowDTSHDPassthru = [preset[@"AudioAllowDTSHDPass"] boolValue];
    self.allowDTSPassthru= [preset[@"AudioAllowDTSPass"] boolValue];
    self.allowMP3Passthru = [preset[@"AudioAllowMP3Pass"] boolValue];

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

        newTrack.drc = [track[@"AudioTrackDRCSlider"] floatValue];
        newTrack.gain = [track[@"AudioTrackGainSlider"] intValue];
        [self.tracksArray addObject:newTrack];
        [newTrack release];
    }
}

- (void)prepareAudioDefaultsForPreset:(NSMutableDictionary *)preset
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
    preset[@"AudioLanguageList"] = [[self.trackSelectionLanguages copy] autorelease];

    // Passthru settings
    preset[@"AudioAllowAACPass"] = @(self.allowAACPassthru);
    preset[@"AudioAllowAC3Pass"] = @(self.allowAC3Passthru);
    preset[@"AudioAllowDTSHDPass"] = @(self.allowDTSHDPassthru);
    preset[@"AudioAllowDTSPass"] = @(self.allowDTSPassthru);
    preset[@"AudioAllowMP3Pass"] = @(self.allowMP3Passthru);

    preset[@"AudioEncoderFallback"] = @(hb_audio_encoder_get_name(self.encoderFallback));

    preset[@"AudioSecondaryEncoderMode"] = @(self.secondaryEncoderMode);

    NSMutableArray *audioList = [[NSMutableArray alloc] init];

    for (HBAudioTrackPreset *track in self.tracksArray)
    {
        NSString *sampleRate = @"Auto";
        if (hb_audio_samplerate_get_name(track.sampleRate))
        {
            sampleRate = @(hb_audio_samplerate_get_name(track.sampleRate));
        }
        NSDictionary *newTrack = @{@"AudioEncoder": @(hb_audio_encoder_get_name(track.encoder)),
                                   @"AudioMixdown": @(hb_mixdown_get_name(track.mixdown)),
                                   @"AudioSamplerate": sampleRate,
                                   @"AudioBitrate": @(track.bitRate),
                                   @"AudioTrackDRCSlider": @(track.drc),
                                   @"AudioTrackGainSlider": @(track.gain)};

        [audioList addObject:newTrack];
    }

    preset[@"AudioList"] = audioList;
    [audioList release];
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

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBAudioDefaultsVersion"];

    encodeInteger(_trackSelectionBehavior);
    encodeObject(_trackSelectionLanguages);

    encodeObject(_tracksArray);

    encodeBool(_allowAACPassthru);
    encodeBool(_allowAC3Passthru);
    encodeBool(_allowDTSHDPassthru);
    encodeBool(_allowDTSPassthru);
    encodeBool(_allowMP3Passthru);

    encodeInt(_encoderFallback);
    encodeBool(_secondaryEncoderMode);
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInteger(_trackSelectionBehavior);
    decodeObject(_trackSelectionLanguages);

    decodeObject(_tracksArray);

    decodeBool(_allowAACPassthru);
    decodeBool(_allowAC3Passthru);
    decodeBool(_allowDTSHDPassthru);
    decodeBool(_allowDTSPassthru);
    decodeBool(_allowMP3Passthru);

    decodeInt(_encoderFallback);
    decodeBool(_secondaryEncoderMode);

    return self;
}

@end
