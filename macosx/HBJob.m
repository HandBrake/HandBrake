/*  HBJob.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob.h"
#import "HBPreset.h"

#import "HBAudio.h"
#import "HBAudioController.h"
#import "HBSubtitlesController.h"

#import "NSCodingMacro.h"

NSString *keyAudioTrackIndex = @"keyAudioTrackIndex";
NSString *keyAudioTrackName = @"keyAudioTrackName";
NSString *keyAudioInputBitrate = @"keyAudioInputBitrate";
NSString *keyAudioInputSampleRate = @"keyAudioInputSampleRate";
NSString *keyAudioInputCodec = @"keyAudioInputCodec";
NSString *keyAudioInputCodecParam = @"keyAudioInputCodecParam";
NSString *keyAudioInputChannelLayout = @"keyAudioInputChannelLayout";
NSString *keyAudioTrackLanguageIsoCode = @"keyAudioTrackLanguageIsoCode";

NSString *HBMixdownChangedNotification         = @"HBMixdownChangedNotification";
NSString *HBContainerChangedNotification       = @"HBContainerChangedNotification";
NSString *keyContainerTag                      = @"keyContainerTag";

@implementation HBJob

- (instancetype)initWithTitle:(HBTitle *)title andPreset:(HBPreset *)preset
{
    self = [super init];
    if (self) {
        NSParameterAssert(title);
        NSParameterAssert(preset);

        _title = title;
        _titleIdx = title.hb_title->index;

        _fileURL = [[NSURL fileURLWithPath:@(title.hb_title->path)] retain];

        _container = HB_MUX_MP4;
        _angle = 1;

        _audioDefaults = [[HBAudioDefaults alloc] init];
        _subtitlesDefaults = [[HBSubtitlesDefaults alloc] init];

        _range = [[HBRange alloc] initWithTitle:title];
        _video = [[HBVideo alloc] initWithJob:self];
        _picture = [[HBPicture alloc] initWithTitle:title];
        _filters = [[HBFilters alloc] init];

        _audioTracks = [[NSMutableArray alloc] init];
        _subtitlesTracks = [[NSMutableArray alloc] init];

        _chapterTitles = [title.chapters mutableCopy];

        [self applyPreset:preset];
    }

    return self;
}

- (void)applyPreset:(HBPreset *)preset
{
    NSDictionary *content = preset.content;

    self.container = hb_container_get_from_name(hb_container_sanitize_name([content[@"FileFormat"] UTF8String]));

    // MP4 specifics options.
    self.mp4HttpOptimize = [content[@"Mp4HttpOptimize"] boolValue];
    self.mp4iPodCompatible = [content[@"Mp4iPodCompatible"] boolValue];

    // Chapter Markers
    self.chaptersEnabled = [content[@"ChapterMarkers"] boolValue];

    [@[self.audioDefaults, self.subtitlesDefaults, self.filters, self.picture, self.video, ] makeObjectsPerformSelector:@selector(applyPreset:)
                                                                                                           withObject:content];
}

- (void)applyCurrentSettingsToPreset:(NSMutableDictionary *)dict
{
    dict[@"FileFormat"] = @(hb_container_get_name(self.container));
    dict[@"ChapterMarkers"] = @(self.chaptersEnabled);
    // MP4 specifics options.
    dict[@"Mp4HttpOptimize"] = @(self.mp4HttpOptimize);
    dict[@"Mp4iPodCompatible"] = @(self.mp4iPodCompatible);

    [@[self.video, self.filters, self.picture, self.audioDefaults, self.subtitlesDefaults] makeObjectsPerformSelector:@selector(writeToPreset:)
                                                                                                           withObject:dict];
}

- (void)setContainer:(int)container
{
    _container = container;
    [self.video containerChanged];

    /* post a notification for any interested observers to indicate that our video container has changed */
    [[NSNotificationCenter defaultCenter] postNotification:
     [NSNotification notificationWithName:HBContainerChangedNotification
                                   object:self
                                 userInfo:@{keyContainerTag: @(self.container)}]];
}

- (void)setTitle:(HBTitle *)title
{
    _title = title;
    self.range.title = title;
    self.picture.title = title;
}

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key
{
    NSSet *retval = nil;

    if ([key isEqualToString:@"mp4OptionsEnabled"])
    {
        retval = [NSSet setWithObjects:@"container", nil];
    }

    return retval;
}

- (void)dealloc
{
    [_audioTracks release];
    [_subtitlesTracks release];

    [_fileURL release];
    [_destURL release];

    [_range release];
    [_video release];
    [_picture release];
    [_filters release];

    [_audioDefaults release];
    [_subtitlesDefaults release];

    [_chapterTitles release];

    [super dealloc];
}

/**
 *  Prepares a hb_job_t
 */
- (hb_job_t *)hb_job
{
    NSAssert(self.title, @"HBJob: calling hb_job without a valid title loaded");

    hb_title_t *title = self.title.hb_title;
    hb_job_t *job = hb_job_init(title);

    hb_job_set_file(job, self.destURL.path.fileSystemRepresentation);

    // Title Angle for dvdnav
    job->angle = self.angle;

    if (self.range.type == HBRangeTypeChapters)
    {
        // Chapter selection
        job->chapter_start = self.range.chapterStart + 1;
        job->chapter_end   = self.range.chapterStop + 1;
    }
    else if (self.range.type == HBRangeTypeSeconds)
    {
        // we are pts based start / stop
        // Point A to Point B. Time to time in seconds.
        // get the start seconds from the start seconds field
        int start_seconds = self.range.secondsStart;
        job->pts_to_start = start_seconds * 90000LL;
        // Stop seconds is actually the duration of encode, so subtract the end seconds from the start seconds
        int stop_seconds = self.range.secondsStop;
        job->pts_to_stop = stop_seconds * 90000LL;
    }
    else if (self.range.type == HBRangeTypeFrames)
    {
        // we are frame based start / stop
        //Point A to Point B. Frame to frame
        // get the start frame from the start frame field
        int start_frame = self.range.frameStart;
        job->frame_to_start = start_frame;
        // get the frame to stop on from the end frame field
        int stop_frame = self.range.frameStop;
        job->frame_to_stop = stop_frame;
    }

    // Format (Muxer) and Video Encoder
    job->mux = self.container;
    job->vcodec = self.video.encoder;

    // We set http optimized mp4 here
    job->mp4_optimize = self.mp4HttpOptimize;

    // We set the chapter marker extraction here based on the format being
    // mpeg4 or mkv and the checkbox being checked.
    if (self.chaptersEnabled)
    {
        job->chapter_markers = 1;

        // now lets get our saved chapter names out the array in the queue file
        // and insert them back into the title chapter list. We have it here,
        // because unless we are inserting chapter markers there is no need to
        // spend the overhead of iterating through the chapter names array imo
        // Also, note that if for some reason we don't apply chapter names, the
        // chapters just come out 001, 002, etc. etc.
        int i = 0;
        for (NSString *name in self.chapterTitles)
        {
            hb_chapter_t *chapter = (hb_chapter_t *) hb_list_item(job->list_chapter, i);
            if (chapter != NULL)
            {
                hb_chapter_set_title(chapter, name.UTF8String);
            }
            i++;
        }
    }
    else
    {
        job->chapter_markers = 0;
    }

    if (job->vcodec == HB_VCODEC_X264 || job->vcodec == HB_VCODEC_X265)
    {
        // iPod 5G atom
        job->ipod_atom = self.mp4iPodCompatible;

        // set fastfirstpass if 2-pass and Turbo are enabled
        if (self.video.twoPass)
        {
            job->fastfirstpass = self.video.turboTwoPass;
        }

        // advanced x264 options
        NSString   *tmpString;
        // translate zero-length strings to NULL for libhb
        const char *encoder_preset  = NULL;
        const char *encoder_tune    = NULL;
        const char *encoder_options = NULL;
        const char *encoder_profile = NULL;
        const char *encoder_level   = NULL;
        if (self.video.advancedOptions)
        {
            // we are using the advanced panel
            if ([(tmpString = self.video.videoOptionExtra) length])
            {
                encoder_options = tmpString.UTF8String;
            }
        }
        else
        {
            // we are using the x264 preset system
            if ([(tmpString = self.video.completeTune) length])
            {
                encoder_tune = [tmpString UTF8String];
            }
            if ([(tmpString = self.video.videoOptionExtra) length])
            {
                encoder_options = [tmpString UTF8String];
            }
            if ([(tmpString = self.video.profile) length])
            {
                encoder_profile = [tmpString UTF8String];
            }
            if ([(tmpString = self.video.level) length])
            {
                encoder_level = [tmpString UTF8String];
            }
            encoder_preset = self.video.preset.UTF8String;
        }
        hb_job_set_encoder_preset (job, encoder_preset);
        hb_job_set_encoder_tune   (job, encoder_tune);
        hb_job_set_encoder_options(job, encoder_options);
        hb_job_set_encoder_profile(job, encoder_profile);
        hb_job_set_encoder_level  (job, encoder_level);
    }
    else if (job->vcodec & HB_VCODEC_FFMPEG_MASK)
    {
        hb_job_set_encoder_options(job, self.video.videoOptionExtra.UTF8String);
    }

    // Picture Size Settings
    job->par.num = self.picture.parWidth;
    job->par.den = self.picture.parHeight;

    // Video settings
    // Framerate
    int fps_mode, fps_num, fps_den;
    if (self.video.frameRate > 0)
    {
        // a specific framerate has been chosen
        fps_num = 27000000;
        fps_den = self.video.frameRate;
        if (self.video.frameRateMode == 1)
        {
            // CFR
            fps_mode = 1;
        }
        else
        {
            // PFR
            fps_mode = 2;
        }
    }
    else
    {
        // same as source
        fps_num = title->vrate.num;
        fps_den = title->vrate.den;
        if (self.video.frameRateMode == 1)
        {
            // CFR
            fps_mode = 1;
        }
        else
        {
            // VFR
            fps_mode = 0;
        }
    }

    switch (self.video.qualityType)
    {
        case 0:
            // ABR
            job->vquality = -1.0;
            job->vbitrate = self.video.avgBitrate;
            break;
        case 1:
            // Constant Quality
            job->vquality = self.video.quality;
            job->vbitrate = 0;
            break;
    }

    job->grayscale = self.filters.grayscale;

    // Map the settings in the dictionaries for the SubtitleList array to match title->list_subtitle
    BOOL one_burned = NO;
    int i = 0;

    for (NSDictionary *subtitleDict in self.subtitlesTracks)
    {
        int subtitle = [subtitleDict[keySubTrackIndex] intValue];
        int force = [subtitleDict[keySubTrackForced] intValue];
        int burned = [subtitleDict[keySubTrackBurned] intValue];
        int def = [subtitleDict[keySubTrackDefault] intValue];

        // if i is 0, then we are in the first item of the subtitles which we need to
        // check for the "Foreign Audio Search" which would be keySubTrackIndex of -1

        // if we are on the first track and using "Foreign Audio Search"
        if (i == 0 && subtitle == -1)
        {
            job->indepth_scan = 1;

            if (burned != 1)
            {
                job->select_subtitle_config.dest = PASSTHRUSUB;
            }
            else
            {
                job->select_subtitle_config.dest = RENDERSUB;
            }

            job->select_subtitle_config.force = force;
            job->select_subtitle_config.default_track = def;
        }
        else
        {
            // if we are getting the subtitles from an external srt file
            if ([subtitleDict[keySubTrackType] intValue] == SRTSUB)
            {
                hb_subtitle_config_t sub_config;

                sub_config.offset = [subtitleDict[keySubTrackSrtOffset] intValue];

                // we need to srncpy file name and codeset
                strncpy(sub_config.src_filename, [subtitleDict[keySubTrackSrtFilePath] UTF8String], 255);
                sub_config.src_filename[255] = 0;
                strncpy(sub_config.src_codeset, [subtitleDict[keySubTrackSrtCharCode] UTF8String], 39);
                sub_config.src_codeset[39] = 0;

                if (!burned && hb_subtitle_can_pass(SRTSUB, job->mux))
                {
                    sub_config.dest = PASSTHRUSUB;
                }
                else if (hb_subtitle_can_burn(SRTSUB))
                {
                    // Only allow one subtitle to be burned into the video
                    if (one_burned)
                        continue;
                    one_burned = TRUE;
                    sub_config.dest = RENDERSUB;
                }

                sub_config.force = 0;
                sub_config.default_track = def;
                hb_srt_add( job, &sub_config, [subtitleDict[keySubTrackLanguageIsoCode] UTF8String]);
                continue;
            }

            // We are setting a source subtitle so access the source subtitle info
            hb_subtitle_t * subt = (hb_subtitle_t *) hb_list_item(title->list_subtitle, subtitle);

            if (subt != NULL)
            {
                hb_subtitle_config_t sub_config = subt->config;

                if (!burned && hb_subtitle_can_pass(subt->source, job->mux))
                {
                    sub_config.dest = PASSTHRUSUB;
                }
                else if (hb_subtitle_can_burn(subt->source))
                {
                    // Only allow one subtitle to be burned into the video
                    if (one_burned)
                        continue;
                    one_burned = TRUE;
                    sub_config.dest = RENDERSUB;
                }

                sub_config.force = force;
                sub_config.default_track = def;
                hb_subtitle_add(job, &sub_config, subtitle);
            }
        }
        i++;
    }

    if (one_burned)
    {
        hb_filter_object_t *filter = hb_filter_init( HB_FILTER_RENDER_SUB );
        hb_add_filter( job, filter, [[NSString stringWithFormat:@"%d:%d:%d:%d",
                                      job->crop[0], job->crop[1],
                                      job->crop[2], job->crop[3]] UTF8String] );
    }

    // Audio Defaults
    job->acodec_copy_mask = 0;

    if (self.audioDefaults.allowAACPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_FFAAC;
    }
    if (self.audioDefaults.allowAC3Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_AC3;
    }
    if (self.audioDefaults.allowDTSHDPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA_HD;
    }
    if (self.audioDefaults.allowDTSPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA;
    }
    if (self.audioDefaults.allowMP3Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_MP3;
    }
    job->acodec_fallback = self.audioDefaults.encoderFallback;

    // Audio tracks and mixdowns
    // Now lets add our new tracks to the audio list here
    for (HBAudio *audioTrack in self.audioTracks)
    {
        if (audioTrack.enabled)
        {
            hb_audio_config_t *audio = (hb_audio_config_t *)calloc(1, sizeof(*audio));
            hb_audio_config_init(audio);

            NSNumber *sampleRateToUse = ([audioTrack.sampleRate[keyAudioSamplerate] intValue] == 0 ?
                                         audioTrack.track[keyAudioInputSampleRate] :
                                         audioTrack.sampleRate[keyAudioSamplerate]);

            audio->in.track            = [audioTrack.track[keyAudioTrackIndex] intValue] -1;

            // We go ahead and assign values to our audio->out.<properties>
            audio->out.track                     = audio->in.track;
            audio->out.codec                     = [audioTrack.codec[keyAudioCodec] intValue];
            audio->out.compression_level         = hb_audio_compression_get_default(audio->out.codec);
            audio->out.mixdown                   = [audioTrack.mixdown[keyAudioMixdown] intValue];
            audio->out.normalize_mix_level       = 0;
            audio->out.bitrate                   = [audioTrack.bitRate[keyAudioBitrate] intValue];
            audio->out.samplerate                = [sampleRateToUse intValue];
            audio->out.dither_method             = hb_audio_dither_get_default();

            // output is not passthru so apply gain
            if (!([[audioTrack codec][keyAudioCodec] intValue] & HB_ACODEC_PASS_FLAG))
            {
                audio->out.gain = [audioTrack.gain doubleValue];
            }
            else
            {
                // output is passthru - the Gain dial is disabled so don't apply its value
                audio->out.gain = 0;
            }

            if (hb_audio_can_apply_drc([audioTrack.track[keyAudioInputCodec] intValue],
                                       [audioTrack.track[keyAudioInputCodecParam] intValue],
                                       [audioTrack.codec[keyAudioCodec] intValue]))
            {
                audio->out.dynamic_range_compression = [audioTrack.drc doubleValue];
            }
            else
            {
                // source isn't AC3 or output is passthru - the DRC dial is disabled so don't apply its value
                audio->out.dynamic_range_compression = 0;
            }

            hb_audio_add(job, audio);
            free(audio);
        }
    }

    // Now lets call the filters if applicable.
    // The order of the filters is critical

    // Detelecine
    hb_filter_object_t *filter;
    if (self.filters.detelecine == 1)
    {
        filter = hb_filter_init(HB_FILTER_DETELECINE);
        // use a custom detelecine string
        hb_add_filter(job, filter, self.filters.detelecineCustomString.UTF8String);
    }
    else if (self.filters.detelecine == 2)
    {
        filter = hb_filter_init(HB_FILTER_DETELECINE);
        // Use libhb's default values
        hb_add_filter(job, filter, NULL);
    }

    if (self.filters.useDecomb && self.filters.decomb)
    {
        // Decomb
        filter = hb_filter_init(HB_FILTER_DECOMB);
        if (self.filters.decomb == 1)
        {
            // use a custom decomb string */
            hb_add_filter(job, filter, self.filters.decombCustomString.UTF8String);
        }
        else if (self.filters.decomb == 2)
        {
            // use libhb defaults
            hb_add_filter(job, filter, NULL);
        }
        else if (self.filters.decomb == 3)
        {
            // use old defaults (decomb fast)
            hb_add_filter(job, filter, "7:2:6:9:1:80");
        }
        else if (self.filters.decomb == 4)
        {
            // decomb 3 with bobbing enabled
            hb_add_filter(job, filter, "455");
        }
    }
    else if (self.filters.deinterlace)
    {
        // Deinterlace
        filter = hb_filter_init(HB_FILTER_DEINTERLACE);
        if (self.filters.deinterlace == 1)
        {
            // we add the custom string if present
            hb_add_filter(job, filter, self.filters.deinterlaceCustomString.UTF8String);
        }
        else if (self.filters.deinterlace == 2)
        {
            // Run old deinterlacer fd by default
            hb_add_filter(job, filter, "0");
        }
        else if (self.filters.deinterlace == 3)
        {
            // Yadif mode 0 (without spatial deinterlacing)
            hb_add_filter(job, filter, "1");
        }
        else if (self.filters.deinterlace == 4)
        {
            // Yadif (with spatial deinterlacing)
            hb_add_filter(job, filter, "3");
        }
        else if (self.filters.deinterlace == 5)
        {
            // Yadif (with spatial deinterlacing and bobbing)
            hb_add_filter(job, filter, "15");
        }
    }
    // Denoise
    if (![self.filters.denoise isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_HQDN3D;
        if ([self.filters.denoise isEqualToString:@"nlmeans"])
            filter_id = HB_FILTER_NLMEANS;

        if ([self.filters.denoisePreset isEqualToString:@"none"])
        {
            const char *filter_str;
            filter_str = self.filters.denoiseCustomString.UTF8String;
            filter = hb_filter_init(filter_id);
            hb_add_filter(job, filter, filter_str);
        }
        else
        {
            const char *filter_str, *preset, *tune;
            preset = self.filters.denoisePreset.UTF8String;
            tune = self.filters.denoiseTune.UTF8String;
            filter_str = hb_generate_filter_settings(filter_id, preset, tune);
            filter = hb_filter_init(filter_id);
            hb_add_filter(job, filter, filter_str);
        }
    }

    // Deblock  (uses pp7 default)
    // NOTE: even though there is a valid deblock setting of 0 for the filter, for
    // the macgui's purposes a value of 0 actually means to not even use the filter
    // current hb_filter_deblock.settings valid ranges are from 5 - 15
    if (self.filters.deblock != 0)
    {
        filter = hb_filter_init(HB_FILTER_DEBLOCK);
        hb_add_filter(job, filter, [NSString stringWithFormat:@"%ld", (long)self.filters.deblock].UTF8String);
    }

    // Add Crop/Scale filter
    filter = hb_filter_init(HB_FILTER_CROP_SCALE);
    hb_add_filter( job, filter, [NSString stringWithFormat:@"%d:%d:%d:%d:%d:%d",
                                  self.picture.width, self.picture.height,
                                  self.picture.cropTop, self.picture.cropBottom,
                                  self.picture.cropLeft, self.picture.cropRight].UTF8String);

    // Add framerate shaping filter
    filter = hb_filter_init(HB_FILTER_VFR);
    hb_add_filter(job, filter, [[NSString stringWithFormat:@"%d:%d:%d",
                                 fps_mode, fps_num, fps_den] UTF8String]);

    return job;
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBJob *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_state = HBJobStateReady;
        copy->_titleIdx = _titleIdx;
        copy->_pidId = _pidId;

        copy->_fileURL = [_fileURL copy];
        copy->_destURL = [_destURL copy];

        copy->_container = _container;
        copy->_angle = _angle;
        copy->_mp4HttpOptimize = _mp4HttpOptimize;
        copy->_mp4iPodCompatible = _mp4iPodCompatible;

        copy->_range = [_range copy];
        copy->_video = [_video copy];
        copy->_picture = [_picture copy];
        copy->_filters = [_filters copy];

        // Copy the tracks, but not the last one because it's empty.
        copy->_audioTracks = [[NSMutableArray alloc] init];
        [_audioTracks enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
            if (idx < _audioTracks.count - 1)
            {
                [copy->_audioTracks addObject:[[obj copy] autorelease]];
            }
        }];
        copy->_subtitlesTracks = [[NSMutableArray alloc] init];
        [_subtitlesTracks enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
            if (idx < _subtitlesTracks.count - 1)
            {
                [copy->_subtitlesTracks addObject:[[obj copy] autorelease]];
            }
        }];
        copy->_chaptersEnabled = _chaptersEnabled;
        copy->_chapterTitles = [[NSMutableArray alloc] initWithArray:_chapterTitles copyItems:YES];

        copy->_audioDefaults = [_audioDefaults copy];
        copy->_subtitlesDefaults = [_subtitlesDefaults copy];
    }

    return copy;
}

#pragma mark - NSCoding

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:1 forKey:@"HBVideoVersion"];

    encodeInt(_state);
    encodeInt(_titleIdx);
    encodeInt(_pidId);

    encodeObject(_fileURL);
    encodeObject(_destURL);

    encodeInt(_container);
    encodeInt(_angle);
    encodeBool(_mp4HttpOptimize);
    encodeBool(_mp4iPodCompatible);

    encodeObject(_range);
    encodeObject(_video);
    encodeObject(_picture);
    encodeObject(_filters);

    encodeObject(_audioTracks);
    encodeObject(_subtitlesTracks);

    encodeBool(_chaptersEnabled);
    encodeObject(_chapterTitles);

    encodeObject(_audioDefaults);
    encodeObject(_subtitlesDefaults);
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInt(_state);
    decodeInt(_titleIdx);
    decodeInt(_pidId);

    decodeObject(_fileURL);
    decodeObject(_destURL);

    decodeInt(_container);
    decodeInt(_angle);
    decodeBool(_mp4HttpOptimize);
    decodeBool(_mp4iPodCompatible);

    decodeObject(_range);
    decodeObject(_video);
    decodeObject(_picture);
    decodeObject(_filters);

    _video.job = self;

    decodeObject(_audioTracks);
    decodeObject(_subtitlesTracks);

    decodeBool(_chaptersEnabled);
    decodeObject(_chapterTitles);

    decodeObject(_audioDefaults);
    decodeObject(_subtitlesDefaults);

    return self;
}

@end
