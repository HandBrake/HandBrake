/*  HBJob+HBJobConversion.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJob+HBJobConversion.h"

#import "HBAudioDefaults.h"
#import "HBAudioTrack.h"

#import "HBSubtitlesTrack.h"

#import "HBChapter.h"

#import "HBTitle+Private.h"
#import "HBMutablePreset.h"

#import "HBRange.h"
#import "HBVideo.h"
#import "HBPicture.h"
#import "HBFilters.h"
#import "HBAudio.h"
#import "HBSubtitles.h"

#import "NSDictionary+HBAdditions.h"

@implementation HBJob (HBJobConversion)

- (NSDictionary *)jobDict
{
    NSAssert(self.title, @"HBJob: calling jobDict without a valid title loaded");

    HBMutablePreset *preset = [[HBMutablePreset alloc] init];
    [self writeToPreset:preset];

    return [self.title jobSettingsWithPreset:preset];
}

/**
 *  Prepares a hb_job_t
 */
- (hb_job_t *)hb_job
{
    NSAssert(self.title, @"HBJob: calling hb_job without a valid title loaded");
    NSAssert(self.destinationURL, @"HBJob: calling hb_job without a valid destination");

    hb_title_t *title = self.title.hb_title;
    hb_job_t *job = hb_job_init(title);

    hb_job_set_file(job, self.destinationURL.fileSystemRepresentation);

    if (self.hwDecodeUsage == HBJobHardwareDecoderUsageFullPathOnly)
    {
        job->hw_decode = HB_DECODE_SUPPORT_VIDEOTOOLBOX;
    }
    else if (self.hwDecodeUsage == HBJobHardwareDecoderUsageAlways)
    {
        job->hw_decode = HB_DECODE_SUPPORT_VIDEOTOOLBOX | HB_DECODE_SUPPORT_FORCE_HW;
    }

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

    job->optimize = self.optimize;

    if (self.container & HB_MUX_MASK_MP4)
    {
        job->align_av_start = self.alignAVStart;
    }

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

    if (self.metadataPassthru == NO && job->metadata)
    {
        if (job->metadata->dict)
        {
            hb_dict_clear(job->metadata->dict);
        }
        if (job->metadata->list_coverart)
        {
            int count = hb_list_count(job->metadata->list_coverart);
            for (int i = 0; i < count; i++)
            {
                hb_metadata_rem_coverart(job->metadata, 0);
            }
        }
    }

    if (job->vcodec & HB_VCODEC_H264_MASK)
    {
        // iPod 5G atom
        job->ipod_atom = self.mp4iPodCompatible;
    }

    if (self.video.multiPass && ((self.video.encoder & HB_VCODEC_X264_MASK) ||
                               (self.video.encoder & HB_VCODEC_X265_MASK)))
    {
        job->fastanalysispass = self.video.turboMultiPass;
    }
    job->multipass = self.video.multiPass;

    switch (self.video.passthruHDRDynamicMetadata)
    {
        case HBVideoHDRDynamicMetadataPassthruOff:
            job->passthru_dynamic_hdr_metadata = HB_HDR_DYNAMIC_METADATA_NONE;
            break;
        case HBVideoHDRDynamicMetadataPassthruHDR10Plus:
            job->passthru_dynamic_hdr_metadata = HB_HDR_DYNAMIC_METADATA_HDR10PLUS;
            break;
        case HBVideoHDRDynamicMetadataPassthruDolbyVision:
            job->passthru_dynamic_hdr_metadata = HB_HDR_DYNAMIC_METADATA_DOVI;
            break;
        case HBVideoHDRDynamicMetadataPassthruAll:
        default:
            job->passthru_dynamic_hdr_metadata = HB_HDR_DYNAMIC_METADATA_ALL;
            break;
    }

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
    job->par.num = self.picture.parNum;
    job->par.den = self.picture.parDen;

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
            job->vquality = HB_INVALID_VIDEO_QUALITY;
            job->vbitrate = self.video.avgBitrate;
            break;
        case 1:
            // Constant Quality
            job->vquality = self.video.quality;
            job->vbitrate = 0;
            break;
    }

    // Map the settings in the dictionaries for the SubtitleList array to match title->list_subtitle
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
                // if we are getting the subtitles from an external file
                if (subTrack.type == IMPORTSRT || subTrack.type == IMPORTSSA)
                {
                    if (subTrack.fileURL)
                    {
                        hb_subtitle_config_t sub_config = {0};
                        sub_config.name = subTrack.title.UTF8String;
                        sub_config.offset = subTrack.offset;

                        // we need to strncpy file name and codeset
                        sub_config.src_filename = subTrack.fileURL.fileSystemRepresentation;
                        if (subTrack.charCode)
                        {
                            size_t len = sizeof(sub_config.src_codeset) - 1;
                            strncpy(sub_config.src_codeset, subTrack.charCode.UTF8String, len);
                            sub_config.src_codeset[len] = 0;
                        }

                        if (!subTrack.burnedIn && hb_subtitle_can_pass(subTrack.type, job->mux))
                        {
                            sub_config.dest = PASSTHRUSUB;
                        }
                        else if (hb_subtitle_can_burn(subTrack.type))
                        {
                            sub_config.dest = RENDERSUB;
                        }

                        sub_config.force = 0;
                        sub_config.default_track = subTrack.def;
                        hb_import_subtitle_add(job, &sub_config, subTrack.isoLanguage.UTF8String, subTrack.type);
                    }
                }
                else
                {
                    // We are setting a source subtitle so access the source subtitle info
                    hb_subtitle_t *subt = (hb_subtitle_t *)hb_list_item(title->list_subtitle, sourceIdx);

                    if (subt != NULL)
                    {
                        hb_subtitle_config_t sub_config = subt->config;
                        sub_config.name = subTrack.title.UTF8String;

                        if (!subTrack.burnedIn && hb_subtitle_can_pass(subt->source, job->mux))
                        {
                            sub_config.dest = PASSTHRUSUB;
                        }
                        else if (hb_subtitle_can_burn(subt->source))
                        {
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

    // Audio Defaults (Selection Behavior)
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
    if (audioDefaults.allowMP2Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_MP2_PASS;
    }
    if (audioDefaults.allowMP3Passthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_MP3_PASS;
    }
    if (audioDefaults.allowVorbisPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_VORBIS_PASS;
    }
    if (audioDefaults.allowOpusPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_OPUS_PASS;
    }
    if (audioDefaults.allowTrueHDPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_TRUEHD_PASS;
    }
    if (audioDefaults.allowALACPassthru)
    {
        job->acodec_copy_mask |= HB_ACODEC_ALAC_PASS;
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
        if (audioTrack.isEnabled)
        {
            hb_audio_config_t audio = {0};
            hb_audio_config_init(&audio);

            HBTitleAudioTrack *inputTrack = self.audio.sourceTracks[audioTrack.sourceTrackIdx];

            int sampleRateToUse = (audioTrack.sampleRate == 0 ?
                                   inputTrack.sampleRate :
                                   audioTrack.sampleRate);

            audio.index = (int)audioTrack.sourceTrackIdx - 1;

            // We go ahead and assign values to our audio->out.<properties>
            audio.out.track                     = audio.in.track;
            audio.out.codec                     = audioTrack.encoder;
            audio.out.compression_level         = hb_audio_compression_get_default(audio.out.codec);
            audio.out.mixdown                   = audioTrack.mixdown;
            audio.out.normalize_mix_level       = 0;
            audio.out.bitrate                   = audioTrack.bitRate;
            audio.out.samplerate                = sampleRateToUse;
            audio.out.dither_method             = hb_audio_dither_get_default();
            audio.out.name                      = audioTrack.title.UTF8String;

            // output is not passthru so apply gain
            if (!(audioTrack.encoder & HB_ACODEC_PASS_FLAG))
            {
                audio.out.gain = audioTrack.gain;
            }
            else
            {
                // output is passthru - the Gain dial is disabled so don't apply its value
                audio.out.gain = 0;
            }

            if (hb_audio_can_apply_drc(inputTrack.codec,
                                       inputTrack.codecParam,
                                       audioTrack.encoder))
            {
                audio.out.dynamic_range_compression = audioTrack.drc;
            }
            else
            {
                // source isn't AC3 or output is passthru - the DRC dial is disabled so don't apply its value
                audio.out.dynamic_range_compression = 0;
            }

            hb_audio_add(job, &audio);
        }
    }

    // Now lets call the filters if applicable.
    hb_filter_object_t *filter;

    // Detelecine
    if (![self.filters.detelecine isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_DETELECINE;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             self.filters.detelecine.UTF8String,
                                                             NULL,
                                                             self.filters.detelecineCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_value_free(&filter_dict);
    }

    // Comb Detection
    if (![self.filters.combDetection isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_COMB_DETECT;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             self.filters.combDetection.UTF8String,
                                                             NULL,
                                                             self.filters.combDetectionCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_value_free(&filter_dict);
    }

    // Deinterlace
    if (![self.filters.deinterlace isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_DECOMB;
        if ([self.filters.deinterlace isEqualToString:@"deinterlace"])
        {
            filter_id = HB_FILTER_YADIF;
        }
        else if ([self.filters.deinterlace isEqualToString:@"bwdif"])
        {
            filter_id = HB_FILTER_BWDIF;
        }

        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                            self.filters.deinterlacePreset.UTF8String,
                                                            NULL,
                                                            self.filters.deinterlaceCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_value_free(&filter_dict);
    }

    // Add framerate shaping filter
    filter = hb_filter_init(HB_FILTER_VFR);
    hb_add_filter(job, filter, [[NSString stringWithFormat:@"mode=%d:rate=%d/%d",
                                 fps_mode, fps_num, fps_den] UTF8String]);

    // Deblock
    if (![self.filters.deblock isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_DEBLOCK;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             self.filters.deblock.UTF8String,
                                                             self.filters.deblockTune.UTF8String,
                                                             self.filters.deblockCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_value_free(&filter_dict);
    }

    // Denoise
    if (![self.filters.denoise isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_HQDN3D;
        if ([self.filters.denoise isEqualToString:@"nlmeans"])
        {
            filter_id = HB_FILTER_NLMEANS;
        }

        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                  self.filters.denoisePreset.UTF8String,
                                                  self.filters.denoiseTune.UTF8String,
                                                  self.filters.denoiseCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_dict_free(&filter_dict);
    }

    // Chroma Smooth
    if (![self.filters.chromaSmooth isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_CHROMA_SMOOTH;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             self.filters.chromaSmooth.UTF8String,
                                                             self.filters.chromaSmoothTune.UTF8String,
                                                             self.filters.chromaSmoothCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_value_free(&filter_dict);
    }

    // Add Crop/Scale filter
    filter = hb_filter_init(HB_FILTER_CROP_SCALE);
    hb_add_filter( job, filter,
                   [NSString stringWithFormat:
                    @"width=%d:height=%d:crop-top=%d:crop-bottom=%d:crop-left=%d:crop-right=%d",
                    self.picture.width, self.picture.height,
                    self.picture.cropTop, self.picture.cropBottom,
                    self.picture.cropLeft, self.picture.cropRight].UTF8String);

    // Sharpen
    if (![self.filters.sharpen isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_UNSHARP;
        if ([self.filters.sharpen isEqualToString:@"lapsharp"])
        {
            filter_id = HB_FILTER_LAPSHARP;
        }

        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                  self.filters.sharpenPreset.UTF8String,
                                                  self.filters.sharpenTune.UTF8String,
                                                  self.filters.sharpenCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_dict_free(&filter_dict);
    }

    // Grayscale
    if (self.filters.grayscale)
    {
        filter = hb_filter_init(HB_FILTER_GRAYSCALE);
        hb_add_filter(job, filter, NULL);
    }

    // Rotate
    if (self.picture.angle || self.picture.flip)
    {
        int filter_id = HB_FILTER_ROTATE;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             NULL, NULL,
                                                             [NSString stringWithFormat:@"angle=%d:hflip=%d",
                                                              self.picture.angle, self.picture.flip].UTF8String);

        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_dict_free(&filter_dict);
    }

    // Pad
    if (self.picture.padMode != HBPicturePadModeNone)
    {
        int filter_id = HB_FILTER_PAD;
        NSString *color;
        switch (self.picture.padColorMode) {
            case HBPicturePadColorModeBlack:
                color = @"black";
                break;
            case HBPicturePadColorModeDarkGray:
                color = @"darkslategray";
                break;
            case HBPicturePadColorModeGray:
                color = @"slategray";
                break;
            case HBPicturePadColorModeWhite:
                color = @"white";
                break;
            case HBPicturePadColorModeCustom:
                color = self.picture.padColorCustom;
                break;
        }
        
        NSString *settings = [NSString stringWithFormat:@"color=%@:top=%d:bottom=%d:left=%d:right=%d",
                              color, self.picture.padTop, self.picture.padBottom, self.picture.padLeft, self.picture.padRight];
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id, NULL, NULL, settings.UTF8String);

        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_dict_free(&filter_dict);
    }

    // Colorspace
    if (![self.filters.colorspace isEqualToString:@"off"])
    {
        int filter_id = HB_FILTER_COLORSPACE;
        hb_dict_t *filter_dict = hb_generate_filter_settings(filter_id,
                                                             self.filters.colorspace.UTF8String,
                                                             NULL,
                                                             self.filters.colorspaceCustomString.UTF8String);
        filter = hb_filter_init(filter_id);
        hb_add_filter_dict(job, filter, filter_dict);
        hb_value_free(&filter_dict);
    }

    return job;
}

@end
