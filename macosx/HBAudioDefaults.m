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
    track.undo = self.undo;
    [self insertObject:track inTracksArrayAtIndex:[self countOfTracksArray]];
}

#pragma mark - Properties

- (void)setTrackSelectionBehavior:(HBAudioTrackSelectionBehavior)trackSelectionBehavior
{
    if (trackSelectionBehavior != _trackSelectionBehavior)
    {
        [[self.undo prepareWithInvocationTarget:self] setTrackSelectionBehavior:_trackSelectionBehavior];
    }
    _trackSelectionBehavior = trackSelectionBehavior;
}

- (void)setTrackSelectionLanguages:(NSMutableArray<NSString *> *)trackSelectionLanguages
{
    if (trackSelectionLanguages != _trackSelectionLanguages)
    {
        [[self.undo prepareWithInvocationTarget:self] setTrackSelectionLanguages:_trackSelectionLanguages];
    }
    _trackSelectionLanguages = trackSelectionLanguages;
}

- (void)setAllowAACPassthru:(BOOL)allowAACPassthru
{
    if (allowAACPassthru != _allowAACPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowAACPassthru:_allowAACPassthru];
    }
    _allowAACPassthru = allowAACPassthru;
}

- (void)setAllowAC3Passthru:(BOOL)allowAC3Passthru
{
    if (allowAC3Passthru != _allowAC3Passthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowAC3Passthru:_allowAC3Passthru];
    }
    _allowAC3Passthru = allowAC3Passthru;
}

- (void)setAllowEAC3Passthru:(BOOL)allowEAC3Passthru
{
    if (allowEAC3Passthru != _allowEAC3Passthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowEAC3Passthru:_allowEAC3Passthru];
    }
    _allowEAC3Passthru = allowEAC3Passthru;
}

- (void)setAllowDTSHDPassthru:(BOOL)allowDTSHDPassthru
{
    if (allowDTSHDPassthru != _allowDTSHDPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowDTSHDPassthru:_allowDTSHDPassthru];
    }
    _allowDTSHDPassthru = allowDTSHDPassthru;
}

- (void)setAllowDTSPassthru:(BOOL)allowDTSPassthru
{
    if (allowDTSPassthru != _allowDTSPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowDTSPassthru:_allowDTSPassthru];
    }
    _allowDTSPassthru = allowDTSPassthru;
}

- (void)setAllowMP3Passthru:(BOOL)allowMP3Passthru
{
    if (allowMP3Passthru != _allowMP3Passthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowMP3Passthru:_allowMP3Passthru];
    }
    _allowMP3Passthru = allowMP3Passthru;
}

- (void)setAllowTrueHDPassthru:(BOOL)allowTrueHDPassthru
{
    if (allowTrueHDPassthru != _allowTrueHDPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowTrueHDPassthru:_allowTrueHDPassthru];
    }
    _allowTrueHDPassthru = allowTrueHDPassthru;
}

- (void)setAllowFLACPassthru:(BOOL)allowFLACPassthru
{
    if (allowFLACPassthru != _allowFLACPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowFLACPassthru:_allowFLACPassthru];
    }
    _allowFLACPassthru = allowFLACPassthru;
}

- (void)setEncoderFallback:(int)encoderFallback
{
    if (encoderFallback != _encoderFallback)
    {
        [[self.undo prepareWithInvocationTarget:self] setEncoderFallback:_encoderFallback];
    }
    _encoderFallback = encoderFallback;
}

- (void)setSecondaryEncoderMode:(BOOL)secondaryEncoderMode
{
    if (secondaryEncoderMode != _secondaryEncoderMode)
    {
        [[self.undo prepareWithInvocationTarget:self] setSecondaryEncoderMode:_secondaryEncoderMode];
    }
    _secondaryEncoderMode = secondaryEncoderMode;
}

- (NSArray<NSString *> *)audioEncoderFallbacks
{
    NSMutableArray<NSString *> *fallbacks = [[NSMutableArray alloc] init];
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

#pragma mark - HBPresetCoding

- (void)applyPreset:(HBPreset *)preset
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
        if (self.trackSelectionLanguages.count == 0 || [self.trackSelectionLanguages.firstObject isEqualToString:@"und"])
        {
            if ([[NSUserDefaults standardUserDefaults] stringForKey:@"AlternateLanguage"])
            {
                NSString *lang = [self isoCodeForNativeLang:[[NSUserDefaults standardUserDefaults] stringForKey:@"AlternateLanguage"]];
                if (lang)
                {
                    [self.trackSelectionLanguages insertObject:lang atIndex:0];
                }
            }
            if ([[NSUserDefaults standardUserDefaults] stringForKey:@"DefaultLanguage"])
            {
                NSString *lang = [self isoCodeForNativeLang:[[NSUserDefaults standardUserDefaults] stringForKey:@"DefaultLanguage"]];
                if (lang)
                {
                    [self.trackSelectionLanguages insertObject:lang atIndex:0];
                }
            }
        }
    }

    // Auto Passthru settings
    // first, disable all encoders
    self.allowAACPassthru    = NO;
    self.allowAC3Passthru    = NO;
    self.allowDTSPassthru    = NO;
    self.allowDTSHDPassthru  = NO;
    self.allowEAC3Passthru   = NO;
    self.allowFLACPassthru   = NO;
    self.allowMP3Passthru    = NO;
    self.allowTrueHDPassthru = NO;

    // then, enable allowed passthru encoders
    for (NSString *copyMask in preset[@"AudioCopyMask"])
    {
        int allowedPassthru = hb_audio_encoder_get_from_name(copyMask.UTF8String);
        if (allowedPassthru & HB_ACODEC_PASS_FLAG)
        {
            switch (allowedPassthru)
            {
                case HB_ACODEC_AAC_PASS:
                    self.allowAACPassthru = YES;
                    break;
                case HB_ACODEC_AC3_PASS:
                    self.allowAC3Passthru = YES;
                    break;
                case HB_ACODEC_DCA_PASS:
                    self.allowDTSPassthru = YES;
                    break;
                case HB_ACODEC_DCA_HD_PASS:
                    self.allowDTSHDPassthru = YES;
                    break;
                case HB_ACODEC_EAC3_PASS:
                    self.allowEAC3Passthru = YES;
                    break;
                case HB_ACODEC_FLAC_PASS:
                    self.allowFLACPassthru = YES;
                    break;
                case HB_ACODEC_MP3_PASS:
                    self.allowMP3Passthru = YES;
                    break;
                case HB_ACODEC_TRUEHD_PASS:
                    self.allowTrueHDPassthru = YES;
                    break;
                default:
                    break;
            }
        }
    }

    self.secondaryEncoderMode = [preset[@"AudioSecondaryEncoderMode"] boolValue];

    if (preset[@"AudioEncoderFallback"])
    {
        // map legacy encoder names via libhb
        self.encoderFallback = hb_audio_encoder_get_from_name([preset[@"AudioEncoderFallback"] UTF8String]);
    }

    while ([self countOfTracksArray])
    {
        [self removeObjectFromTracksArrayAtIndex:0];
    }

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
        [self insertObject:newTrack inTracksArrayAtIndex:[self countOfTracksArray]];
    }
}

- (void)writeToPreset:(HBMutablePreset *)preset
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
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_AAC_PASS))];
    }
    if (self.allowAC3Passthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_AC3_PASS))];
    }
    if (self.allowEAC3Passthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_EAC3_PASS))];
    }
    if (self.allowDTSHDPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_DCA_HD_PASS))];
    }
    if (self.allowDTSPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_DCA_PASS))];
    }
    if (self.allowMP3Passthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_MP3_PASS))];
    }
    if (self.allowTrueHDPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_TRUEHD_PASS))];
    }
    if (self.allowFLACPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_FLAC_PASS))];
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

- (void)setUndo:(NSUndoManager *)undo
{
    _undo = undo;
    [self.tracksArray makeObjectsPerformSelector:@selector(setUndo:) withObject:undo];
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
        copy->_container = _container;
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
    [coder encodeInt:2 forKey:@"HBAudioDefaultsVersion"];

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
    encodeInt(_container);
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
    decodeInt(_container);
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
    [[self.undo prepareWithInvocationTarget:self] removeObjectFromTracksArrayAtIndex:index];
    [self.tracksArray insertObject:track atIndex:index];
}

- (void)removeObjectFromTracksArrayAtIndex:(NSUInteger)index
{
    id obj = self.tracksArray[index];
    [[self.undo prepareWithInvocationTarget:self] insertObject:obj inTracksArrayAtIndex:index];
    [self.tracksArray removeObjectAtIndex:index];
}

@end
