/*  HBAudioSettings.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioDefaults.h"
#import "HBAudioTrackPreset.h"
#import "HBCodingUtilities.h"
#import "HBMutablePreset.h"

#import "handbrake/handbrake.h"
#import "handbrake/lang.h"

@interface HBAudioDefaults ()

@property (nonatomic, readwrite) int container;

@end

@implementation HBAudioDefaults

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _encoderFallback = HB_ACODEC_AC3;
        _trackSelectionLanguages = [[NSMutableArray alloc] init];
        _tracksArray = [[NSMutableArray alloc] init];
        _trackSelectionBehavior = HBAudioTrackSelectionBehaviorFirst;
        _container = HB_MUX_MKV;
        _passthruName = NO;
        _automaticNamingBehavior = HBAudioTrackAutomaticNamingBehaviorNone;
    }
    return self;
}

- (void)addTrack
{
    HBAudioTrackPreset *track = [[HBAudioTrackPreset alloc] initWithContainer:self.container];
    track.undo = self.undo;
    track.fallbackEncoder = self.encoderFallback;
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

- (void)setAllowMP2Passthru:(BOOL)allowMP2Passthru
{
    if (allowMP2Passthru != _allowMP2Passthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowMP2Passthru:_allowMP2Passthru];
    }
    _allowMP2Passthru = allowMP2Passthru;
}

- (void)setAllowMP3Passthru:(BOOL)allowMP3Passthru
{
    if (allowMP3Passthru != _allowMP3Passthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowMP3Passthru:_allowMP3Passthru];
    }
    _allowMP3Passthru = allowMP3Passthru;
}

- (void)setAllowVorbisPassthru:(BOOL)allowVorbisPassthru
{
    if (allowVorbisPassthru != _allowVorbisPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowVorbisPassthru:_allowVorbisPassthru];
    }
    _allowVorbisPassthru = allowVorbisPassthru;
}

- (void)setAllowOpusPassthru:(BOOL)allowOpusPassthru
{
    if (allowOpusPassthru != _allowOpusPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowOpusPassthru:_allowOpusPassthru];
    }
    _allowOpusPassthru = allowOpusPassthru;
}

- (void)setAllowTrueHDPassthru:(BOOL)allowTrueHDPassthru
{
    if (allowTrueHDPassthru != _allowTrueHDPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowTrueHDPassthru:_allowTrueHDPassthru];
    }
    _allowTrueHDPassthru = allowTrueHDPassthru;
}

- (void)setAllowALACPassthru:(BOOL)allowALACPassthru
{
    if (allowALACPassthru != _allowALACPassthru)
    {
        [[self.undo prepareWithInvocationTarget:self] setAllowALACPassthru:_allowALACPassthru];
    }
    _allowALACPassthru = allowALACPassthru;
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

    for (HBAudioTrackPreset *track in self.tracksArray)
    {
        track.fallbackEncoder = encoderFallback;
    }
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
            ((audio_encoder->muxers & self.container) || audio_encoder->codec == HB_ACODEC_NONE))
        {
            [fallbacks addObject:@(audio_encoder->name)];
        }
    }
    return fallbacks;
}

- (void)setPassthruName:(BOOL)passthruName
{
    if (passthruName != _passthruName)
    {
        [[self.undo prepareWithInvocationTarget:self] setPassthruName:_passthruName];
    }
    _passthruName = passthruName;
}

- (void)setautomaticNamingBehavior:(HBAudioTrackAutomaticNamingBehavior)automaticNamingBehavior
{
    if (automaticNamingBehavior != _automaticNamingBehavior)
    {
        [[self.undo prepareWithInvocationTarget:self] setautomaticNamingBehavior:_automaticNamingBehavior];
    }
    _automaticNamingBehavior = automaticNamingBehavior;
}

#pragma mark - HBPresetCoding

- (BOOL)applyPreset:(HBPreset *)preset error:(NSError * __autoreleasing *)outError
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

    // Auto Passthru settings
    // first, disable all encoders
    self.allowAACPassthru    = NO;
    self.allowAC3Passthru    = NO;
    self.allowDTSPassthru    = NO;
    self.allowDTSHDPassthru  = NO;
    self.allowEAC3Passthru   = NO;
    self.allowFLACPassthru   = NO;
    self.allowMP2Passthru    = NO;
    self.allowMP3Passthru    = NO;
    self.allowOpusPassthru   = NO;
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
                case HB_ACODEC_ALAC_PASS:
                    self.allowALACPassthru = YES;
                    break;
                case HB_ACODEC_FLAC_PASS:
                    self.allowFLACPassthru = YES;
                    break;
                case HB_ACODEC_MP2_PASS:
                    self.allowMP2Passthru = YES;
                    break;
                case HB_ACODEC_MP3_PASS:
                    self.allowMP3Passthru = YES;
                    break;
                case HB_ACODEC_VORBIS_PASS:
                    self.allowVorbisPassthru = YES;
                    break;
                case HB_ACODEC_OPUS_PASS:
                    self.allowOpusPassthru = YES;
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

    self.passthruName = [preset[@"AudioTrackNamePassthru"] boolValue];

    NSString *automaticNamingBehavior = preset[@"AudioAutomaticNamingBehavior"];
    if ([automaticNamingBehavior isKindOfClass:[NSString class]])
    {
        if ([automaticNamingBehavior isEqualToString:@"none"])
        {
            self.automaticNamingBehavior = HBAudioTrackAutomaticNamingBehaviorNone;
        }
        else if ([automaticNamingBehavior isEqualToString:@"unnamed"])
        {
            self.automaticNamingBehavior = HBAudioTrackAutomaticNamingBehaviorUnnamed;
        }
        else if ([automaticNamingBehavior isEqualToString:@"all"])
        {
            self.automaticNamingBehavior = HBAudioTrackAutomaticNamingBehaviorAll;
        }
    }

    while ([self countOfTracksArray])
    {
        [self removeObjectFromTracksArrayAtIndex:0];
    }

    for (NSDictionary *track in preset[@"AudioList"])
    {
        HBAudioTrackPreset *newTrack = [[HBAudioTrackPreset alloc] init];
        newTrack.fallbackEncoder = self.encoderFallback;

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

    return YES;
}

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings
{
    [self applyPreset:preset error:NULL];
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
    if (self.allowMP2Passthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_MP2_PASS))];
    }
    if (self.allowMP3Passthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_MP3_PASS))];
    }
    if (self.allowVorbisPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_VORBIS_PASS))];
    }
    if (self.allowOpusPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_OPUS_PASS))];
    }
    if (self.allowTrueHDPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_TRUEHD_PASS))];
    }
    if (self.allowALACPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_ALAC_PASS))];
    }
    if (self.allowFLACPassthru)
    {
        [copyMask addObject:@(hb_audio_encoder_get_short_name(HB_ACODEC_FLAC_PASS))];
    }
    preset[@"AudioCopyMask"] = [copyMask copy];

    preset[@"AudioEncoderFallback"] = @(hb_audio_encoder_get_short_name(self.encoderFallback));

    preset[@"AudioSecondaryEncoderMode"] = @(self.secondaryEncoderMode);

    preset[@"AudioTrackNamePassthru"] = @(self.passthruName);

    switch (self.automaticNamingBehavior)
    {
        case HBAudioTrackAutomaticNamingBehaviorNone:
            preset[@"AudioAutomaticNamingBehavior"] = @"none";
            break;
        case HBAudioTrackAutomaticNamingBehaviorUnnamed:
            preset[@"AudioAutomaticNamingBehavior"] = @"unnamed";
            break;
        case HBAudioTrackAutomaticNamingBehaviorAll:
        default:
            preset[@"AudioAutomaticNamingBehavior"] = @"all";
            break;
    }

    NSMutableArray<NSDictionary *> *audioList = [[NSMutableArray alloc] init];

    for (HBAudioTrackPreset *track in self.tracksArray)
    {
        NSString *sampleRate = @"auto";
        if (hb_audio_samplerate_get_name(track.sampleRate))
        {
            sampleRate = @(hb_audio_samplerate_get_name(track.sampleRate));
        }
        const char *encoderShortName = hb_audio_encoder_get_short_name(track.encoder);
        const char *mixdownShortName = hb_mixdown_get_short_name(track.mixdown);
        if (encoderShortName && mixdownShortName)
        {
            NSDictionary *newTrack = @{@"AudioEncoder": @(encoderShortName),
                                       @"AudioMixdown": @(mixdownShortName),
                                       @"AudioSamplerate": sampleRate,
                                       @"AudioBitrate": @(track.bitRate),
                                       @"AudioTrackDRCSlider": @(track.drc),
                                       @"AudioTrackGainSlider": @(track.gain)};

            [audioList addObject:newTrack];
        }
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
        copy->_allowMP2Passthru = _allowMP2Passthru;
        copy->_allowMP3Passthru = _allowMP3Passthru;
        copy->_allowOpusPassthru = _allowOpusPassthru;
        copy->_allowTrueHDPassthru = _allowTrueHDPassthru;
        copy->_allowFLACPassthru = _allowFLACPassthru;

        copy->_encoderFallback = _encoderFallback;
        copy->_container = _container;
        copy->_secondaryEncoderMode = _secondaryEncoderMode;

        copy->_passthruName = _passthruName;
        copy->_automaticNamingBehavior = _automaticNamingBehavior;
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
    encodeBool(_allowMP2Passthru);
    encodeBool(_allowMP3Passthru);
    encodeBool(_allowOpusPassthru);
    encodeBool(_allowTrueHDPassthru);
    encodeBool(_allowFLACPassthru);

    encodeInt(_encoderFallback);
    encodeInt(_container);
    encodeBool(_secondaryEncoderMode);

    encodeBool(_passthruName);
    encodeInteger(_automaticNamingBehavior);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInteger(_trackSelectionBehavior);
    if (_trackSelectionBehavior < HBAudioTrackSelectionBehaviorNone || _trackSelectionBehavior > HBAudioTrackSelectionBehaviorAll)
    {
        goto fail;
    }

    decodeCollectionOfObjectsOrFail(_trackSelectionLanguages, NSMutableArray, NSString);
    decodeCollectionOfObjectsOrFail(_tracksArray, NSMutableArray, HBAudioTrackPreset);

    decodeBool(_allowAACPassthru);
    decodeBool(_allowAC3Passthru);
    decodeBool(_allowEAC3Passthru);
    decodeBool(_allowDTSHDPassthru);
    decodeBool(_allowDTSPassthru);
    decodeBool(_allowMP2Passthru);
    decodeBool(_allowMP3Passthru);
    decodeBool(_allowOpusPassthru);
    decodeBool(_allowTrueHDPassthru);
    decodeBool(_allowFLACPassthru);

    decodeInt(_encoderFallback); if (_encoderFallback < 0) { goto fail; }
    decodeInt(_container); if (_container != HB_MUX_MP4 && _container != HB_MUX_MKV && _container != HB_MUX_WEBM) { goto fail; }
    decodeBool(_secondaryEncoderMode);

    decodeBool(_passthruName);
    decodeInt(_automaticNamingBehavior);
    if (_automaticNamingBehavior < HBAudioTrackAutomaticNamingBehaviorNone || _automaticNamingBehavior > HBAudioTrackAutomaticNamingBehaviorAll)
    {
        goto fail;
    }


    return self;

fail:
    return nil;
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

- (void)insertObject:(HBAudioTrackPreset *)track inTracksArrayAtIndex:(NSUInteger)index
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
