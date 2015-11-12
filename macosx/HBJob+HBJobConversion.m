/*  HBJob+HBJobConversion.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob+HBJobConversion.h"

#import "HBAudioDefaults.h"
#import "HBAudioTrack.h"

#import "HBSubtitlesTrack.h"

#import "HBChapter.h"

#import "HBTitlePrivate.h"

@implementation HBJob (HBJobConversion)

/**
 *  Prepares a hb_job_t
 */
- (hb_job_t *)hb_job
{
    NSAssert(self.title, @"HBJob: calling hb_job without a valid title loaded");
    NSAssert(self.destURL, @"HBJob: calling hb_job without a valid destination");

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
        job->pts_to_stop = (stop_seconds - start_seconds) * 90000LL;
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
        job->frame_to_stop = stop_frame - start_frame;
    }
    else if (self.range.type == HBRangePreviewIndex)
    {
        job->start_at_preview = self.range.previewIndex;
        job->seek_points = self.range.previewsCount;
        job->pts_to_stop = self.range.ptsToStop;
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
        for (HBChapter *jobChapter in self.chapterTitles)
        {
            hb_chapter_t *chapter = (hb_chapter_t *) hb_list_item(job->list_chapter, i);
            if (chapter != NULL)
            {
                hb_chapter_set_title(chapter, jobChapter.title.UTF8String);
            }
            i++;
        }
    }
    else
    {
        job->chapter_markers = 0;
    }

    if (job->vcodec & HB_VCODEC_H264_MASK)
    {
        // iPod 5G atom
        job->ipod_atom = self.mp4iPodCompatible;
    }

    if (self.video.twoPass && ((self.video.encoder & HB_VCODEC_X264_MASK) ||
                               (self.video.encoder & HB_VCODEC_X265_MASK)))
    {
        job->fastfirstpass = self.video.turboTwoPass;
    }
    job->twopass = self.video.twoPass;

    if (hb_video_encoder_get_presets(self.video.encoder) != NULL)
    {
        // advanced x264/x265 options
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
            // we are using the x264/x265 preset system
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

    // Map the settings in the dictionaries for the SubtitleList array to match title->list_subtitle
    BOOL one_burned = NO;
    for (HBSubtitlesTrack *subTrack in self.subtitles.tracks)
    {
        if (subTrack.isEnabled)
        {
            // Subtract 2 to the source indexes to compensate
            // for the none and foreign audio search tracks.
            int sourceIdx = ((int)subTrack.sourceTrackIdx) - 2;

            // we need to check for the "Foreign Audio Search" which would be have an index of -1
            if (sourceIdx == -1)
            {
                job->indepth_scan = 1;

                if (subTrack.burnedIn)
                {
                    job->select_subtitle_config.dest = RENDERSUB;
                }
                else
                {
                    job->select_subtitle_config.dest = PASSTHRUSUB;
                }

                job->select_subtitle_config.force = subTrack.forcedOnly;
                job->select_subtitle_config.default_track = subTrack.def;
            }
            else
            {
                // if we are getting the subtitles from an external srt file
                if (subTrack.type == SRTSUB)
                {
                    hb_subtitle_config_t sub_config;

                    sub_config.offset = subTrack.offset;

                    // we need to strncpy file name and codeset
                    strncpy(sub_config.src_filename, subTrack.fileURL.path.fileSystemRepresentation, 255);
                    sub_config.src_filename[255] = 0;
                    strncpy(sub_config.src_codeset, subTrack.charCode.UTF8String, 39);
                    sub_config.src_codeset[39] = 0;

                    if (!subTrack.burnedIn && hb_subtitle_can_pass(SRTSUB, job->mux))
                    {
                        sub_config.dest = PASSTHRUSUB;
                    }
                    else if (hb_subtitle_can_burn(SRTSUB))
                    {
                        one_burned = YES;
                        sub_config.dest = RENDERSUB;
                    }

                    sub_config.force = 0;
                    sub_config.default_track = subTrack.def;
                    hb_srt_add( job, &sub_config, subTrack.isoLanguage.UTF8String);
                }
                else
                {
                    // We are setting a source subtitle so access the source subtitle info
                    hb_subtitle_t * subt = (hb_subtitle_t *) hb_list_item(title->list_subtitle, sourceIdx);

                    if (subt != NULL)
                    {
                        hb_subtitle_config_t sub_config = subt->config;

                        if (!subTrack.burnedIn && hb_subtitle_can_pass(subt->source, job->mux))
                        {
                            sub_config.dest = PASSTHRUSUB;
                        }
                        else if (hb_subtitle_can_burn(subt->source))
                        {
                            one_burned = YES;
                            sub_config.dest = RENDERSUB;
                        }

                        sub_config.force = subTrack.forcedOnly;
                        sub_config.default_track = subTrack.def;
                        hb_subtitle_add(job, &sub_config, sourceIdx);
                    }
                }
            }
        }
    }

    if (one_burned)
    {
        hb_filter_object_t *filter = hb_filter_init( HB_FILTER_RENDER_SUB );
        hb_add_filter(job, filter, [NSString stringWithFormat:@"%d:%d:%d:%d",
                                      self.picture.cropTop, self.picture.cropBottom,
                                      self.picture.cropLeft, self.picture.cropRight].UTF8String);
    }

    // Audio Defaults
    job->acodec_copy_mask = 0;

    HBAudioDefaults *audioDefaults = self.audio.defaults;

    if (audioDefaults.allowAACPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_AAC_PASS;
    }
    if (audioDefaults.allowAC3Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_AC3_PASS;
    }
    if (audioDefaults.allowEAC3Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_EAC3_PASS;
    }
    if (audioDefaults.allowDTSHDPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA_HD_PASS;
    }
    if (audioDefaults.allowDTSPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_DCA_PASS;
    }
    if (audioDefaults.allowMP3Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_MP3_PASS;
    }
    if (audioDefaults.allowTrueHDPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_TRUEHD_PASS;
    }
    if (audioDefaults.allowFLACPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_FLAC_PASS;
    }

    job->acodec_fallback = audioDefaults.encoderFallback;

    // Audio tracks and mixdowns
    // Now lets add our new tracks to the audio list here
    for (HBAudioTrack *audioTrack in self.audio.tracks)
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
                audio->out.gain = audioTrack.gain;
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
                audio->out.dynamic_range_compression = audioTrack.drc;
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
    if (![self.filters.detelecine isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_DETELECINE;
        const char *filter_str = hb_generate_filter_settings(filter_id,
                                                             self.filters.detelecine.UTF8String,
                                                             self.filters.detelecineCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter(job, filter, filter_str);
    }

    // Deinterlace
    if (![self.filters.deinterlace isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_DECOMB;
        if ([self.filters.deinterlace isEqualToString:@"deinterlace"])
        {
            filter_id = HB_FILTER_DEINTERLACE;
        }

        const char *filter_str = hb_generate_filter_settings(filter_id,
                                                             self.filters.deinterlacePreset.UTF8String,
                                                             self.filters.deinterlaceCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter(job, filter, filter_str);
    }

    // Denoise
    if (![self.filters.denoise isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_HQDN3D;
        if ([self.filters.denoise isEqualToString:@"nlmeans"])
            filter_id = HB_FILTER_NLMEANS;

        if ([self.filters.denoisePreset isEqualToString:@"custom"])
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

    // Deblock (uses pp7 default)
    if (self.filters.deblock)
    {
        filter = hb_filter_init(HB_FILTER_DEBLOCK);
        hb_add_filter(job, filter, [NSString stringWithFormat:@"%d", self.filters.deblock].UTF8String);
    }

    // Add Crop/Scale filter
    filter = hb_filter_init(HB_FILTER_CROP_SCALE);
    hb_add_filter( job, filter, [NSString stringWithFormat:@"%d:%d:%d:%d:%d:%d",
                                 self.picture.width, self.picture.height,
                                 self.picture.cropTop, self.picture.cropBottom,
                                 self.picture.cropLeft, self.picture.cropRight].UTF8String);
    
    // Add grayscale filter
    if (self.filters.grayscale)
    {
        filter = hb_filter_init(HB_FILTER_GRAYSCALE);
        hb_add_filter(job, filter, NULL);
    }

    // Add framerate shaping filter
    filter = hb_filter_init(HB_FILTER_VFR);
    hb_add_filter(job, filter, [[NSString stringWithFormat:@"%d:%d:%d",
                                 fps_mode, fps_num, fps_den] UTF8String]);
    
    return job;
}

@end
