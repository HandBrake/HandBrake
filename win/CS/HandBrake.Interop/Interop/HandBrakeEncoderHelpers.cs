// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeEncoderHelpers.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The encoders.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;
    using HandBrake.Interop.Interop.Helpers;
    using HandBrake.Interop.Interop.Model;
    using HandBrake.Interop.Interop.Model.Encoding;
    using HandBrake.Interop.Interop.Providers;
    using HandBrake.Interop.Interop.Providers.Interfaces;

    /// <summary>
    /// The encoders.
    /// </summary>
    public static class HandBrakeEncoderHelpers
    {
        private static IHbFunctions hbFunctions;
        private static List<HBAudioEncoder> audioEncoders;
        private static List<HBVideoEncoder> videoEncoders;
        private static List<HBRate> videoFramerates;
        private static List<HBMixdown> mixdowns;
        private static List<HBContainer> containers;
        private static List<int> audioBitrates;
        private static List<HBRate> audioSampleRates;

        /// <summary>
        /// Initializes static members of the HandBrakeEncoderHelpers class.
        /// </summary>
        static HandBrakeEncoderHelpers()
        {
            IHbFunctionsProvider hbFunctionsProvider = new HbFunctionsProvider();
            hbFunctions = hbFunctionsProvider.GetHbFunctionsWrapper();

            if (!HandBrakeUtils.IsInitialised())
            {
                throw new Exception("Please Initialise with HandBrakeUtils.EnsureGlobalInit before using!");
            }
        }

        /// <summary>
        /// Gets a list of supported audio encoders.
        /// </summary>
        public static List<HBAudioEncoder> AudioEncoders
        {
            get
            {
                if (audioEncoders == null)
                {
                    audioEncoders = InteropUtilities.ToListFromIterator<hb_encoder_s, HBAudioEncoder>(hbFunctions.hb_audio_encoder_get_next, HandBrakeUnitConversionHelpers.NativeToAudioEncoder);
                }

                return audioEncoders;
            }
        }

        /// <summary>
        /// Gets a list of supported video encoders.
        /// </summary>
        public static List<HBVideoEncoder> VideoEncoders
        {
            get
            {
                if (videoEncoders == null)
                {
                    videoEncoders = InteropUtilities.ToListFromIterator<hb_encoder_s, HBVideoEncoder>(hbFunctions.hb_video_encoder_get_next, HandBrakeUnitConversionHelpers.NativeToVideoEncoder);
                }

                return videoEncoders;
            }
        }

        /// <summary>
        /// Gets a list of supported video framerates (in pts).
        /// </summary>
        public static List<HBRate> VideoFramerates
        {
            get
            {
                if (videoFramerates == null)
                {
                    videoFramerates = InteropUtilities.ToListFromIterator<hb_rate_s, HBRate>(hbFunctions.hb_video_framerate_get_next, HandBrakeUnitConversionHelpers.NativeToRate);
                }

                return videoFramerates;
            }
        }

        /// <summary>
        /// Gets a list of supported mixdowns.
        /// </summary>
        public static List<HBMixdown> Mixdowns
        {
            get
            {
                if (mixdowns == null)
                {
                    mixdowns = InteropUtilities.ToListFromIterator<hb_mixdown_s, HBMixdown>(hbFunctions.hb_mixdown_get_next, HandBrakeUnitConversionHelpers.NativeToMixdown);
                }

                return mixdowns;
            }
        }

        /// <summary>
        /// Gets a list of supported audio bitrates.
        /// </summary>
        public static List<int> AudioBitrates
        {
            get
            {
                if (audioBitrates == null)
                {
                    audioBitrates = InteropUtilities.ToListFromIterator<hb_rate_s, int>(hbFunctions.hb_audio_bitrate_get_next, b => b.rate);
                }

                return audioBitrates;
            }
        }

        /// <summary>
        /// Gets a list of supported audio sample rates (in Hz).
        /// </summary>
        public static List<HBRate> AudioSampleRates
        {
            get
            {
                if (audioSampleRates == null)
                {
                    audioSampleRates = InteropUtilities.ToListFromIterator<hb_rate_s, HBRate>(hbFunctions.hb_audio_samplerate_get_next, HandBrakeUnitConversionHelpers.NativeToRate);
                }

                return audioSampleRates;
            }
        }

        /// <summary>
        /// Gets a list of supported containers.
        /// </summary>
        public static List<HBContainer> Containers
        {
            get
            {
                if (containers == null)
                {
                    containers = InteropUtilities.ToListFromIterator<hb_container_s, HBContainer>(hbFunctions.hb_container_get_next, HandBrakeUnitConversionHelpers.NativeToContainer);
                }

                return containers;
            }
        }

        /// <summary>
        /// Gets a value indicating whether SRT subtitles can be burnt in.
        /// </summary>
        public static bool CanBurnSrt
        {
            get
            {
                return hbFunctions.hb_subtitle_can_burn((int)hb_subtitle_s_subsource.IMPORTSRT) > 0;
            }
        }

        /// <summary>
        /// Gets a value indicating whether SRT subtitles can be burnt in.
        /// </summary>
        public static bool CanBurnSSA
        {
            get
            {
                return hbFunctions.hb_subtitle_can_burn((int)hb_subtitle_s_subsource.IMPORTSSA) > 0;
            }
        }

        /// <summary>
        /// Gets the audio encoder with the specified short name.
        /// </summary>
        /// <param name="shortName">
        /// The name of the audio encoder.
        /// </param>
        /// <returns>
        /// The requested audio encoder.
        /// </returns>
        public static HBAudioEncoder GetAudioEncoder(string shortName)
        {
            return AudioEncoders.SingleOrDefault(e => e.ShortName == shortName);
        }

        /// <summary>
        /// Gets the audio encoder with the specified codec ID.
        /// </summary>
        /// <param name="codecId">
        /// The ID of the audio encoder.
        /// </param>
        /// <returns>
        /// The requested audio encoder.
        /// </returns>
        public static HBAudioEncoder GetAudioEncoder(int codecId)
        {
            return AudioEncoders.SingleOrDefault(e => e.Id == codecId);
        }

        /// <summary>
        /// Gets the video encoder with the specified short name.
        /// </summary>
        /// <param name="shortName">
        /// The name of the video encoder.
        /// </param>
        /// <returns>
        /// The requested video encoder.
        /// </returns>
        public static HBVideoEncoder GetVideoEncoder(string shortName)
        {
            return VideoEncoders.SingleOrDefault(e => e.ShortName == shortName);
        }

        /// <summary>
        /// Gets the mixdown with the specified short name.
        /// </summary>
        /// <param name="shortName">
        /// The name of the mixdown.
        /// </param>
        /// <returns>
        /// The requested mixdown.
        /// </returns>
        public static HBMixdown GetMixdown(string shortName)
        {
            return Mixdowns.SingleOrDefault(m => m.ShortName == shortName);
        }

        /// <summary>
        /// Gets the mixdown with the specified ID.
        /// </summary>
        /// <param name="id">The mixdown ID.</param>
        /// <returns>The requested mixdown.</returns>
        public static HBMixdown GetMixdown(int id)
        {
            return Mixdowns.SingleOrDefault(m => m.Id == id);
        }

        /// <summary>
        /// Gets the container with the specified short name.
        /// </summary>
        /// <param name="shortName">
        /// The name of the container.
        /// </param>
        /// <returns>
        /// The requested container.
        /// </returns>
        public static HBContainer GetContainer(string shortName)
        {
            return Containers.SingleOrDefault(c => c.ShortName == shortName);
        }

        /// <summary>
        /// Returns true if the subtitle source type can be set to forced only.
        /// </summary>
        /// <param name="source">
        /// The subtitle source type (SSA, VobSub, etc)
        /// </param>
        /// <returns>
        /// True if the subtitle source type can be set to forced only.
        /// </returns>
        public static bool SubtitleCanSetForcedOnly(int source)
        {
            return hbFunctions.hb_subtitle_can_force(source) > 0;
        }

        /// <summary>
        /// Returns true if the subtitle source type can be burned in.
        /// </summary>
        /// <param name="source">
        /// The subtitle source type (SSA, VobSub, etc)
        /// </param>
        /// <returns>
        /// True if the subtitle source type can be burned in.
        /// </returns>
        public static bool SubtitleCanBurn(int source)
        {
            return hbFunctions.hb_subtitle_can_burn(source) > 0;
        }

        /// <summary>
        /// Returns true if the subtitle type can be passed through using the given muxer.
        /// </summary>
        /// <param name="subtitleSourceType">
        /// The subtitle source type (SSA, VobSub, etc)
        /// </param>
        /// <param name="muxer">
        /// The ID of the muxer.
        /// </param>
        /// <returns>
        /// True if the subtitle type can be passed through with the given muxer.
        /// </returns>
        public static bool SubtitleCanPassthrough(int subtitleSourceType, int muxer)
        {
            return hbFunctions.hb_subtitle_can_pass(subtitleSourceType, muxer) > 0;
        }

        /// <summary>
        /// Gets the subtitle source type's name.
        /// </summary>
        /// <param name="source">
        /// The subtitle source type (SSA, VobSub, etc).
        /// </param>
        /// <returns>
        /// The name of the subtitle source.
        /// </returns>
        public static string GetSubtitleSourceName(int source)
        {
            switch ((hb_subtitle_s_subsource)source)
            {
                case hb_subtitle_s_subsource.CC608SUB:
                    return "CC608";
                case hb_subtitle_s_subsource.CC708SUB:
                    return "CC708";
                case hb_subtitle_s_subsource.IMPORTSRT:
                    return "SRT";
                case hb_subtitle_s_subsource.IMPORTSSA:
                case hb_subtitle_s_subsource.SSASUB:
                    return "SSA";
                case hb_subtitle_s_subsource.TX3GSUB:
                    return "TX3G";
                case hb_subtitle_s_subsource.UTF8SUB:
                    return "UTF8";
                case hb_subtitle_s_subsource.VOBSUB:
                    return "VobSub";
                case hb_subtitle_s_subsource.PGSSUB:
                    return "PGS";
                default:
                    return string.Empty;
            }
        }

        /// <summary>
        /// Determines if the given encoder is compatible with the given track.
        /// </summary>
        /// <param name="codecId">
        /// The codec Id.
        /// </param>
        /// <param name="encoder">
        /// The encoder to examine.
        /// </param>
        /// <returns>
        /// True if the given encoder is comatible with the given audio track.
        /// </returns>
        /// <remarks>
        /// Only works with passthrough encoders.
        /// </remarks>
        public static bool AudioEncoderIsCompatible(int codecId, HBAudioEncoder encoder)
        {
            return (codecId & encoder.Id) > 0;
        }

        /// <summary>
        /// Determines if the given mixdown supports the given channel layout.
        /// </summary>
        /// <param name="mixdown">
        /// The mixdown to evaluate.
        /// </param>
        /// <param name="layout">
        /// The channel layout to evaluate.
        /// </param>
        /// <returns>
        /// True if the mixdown supports the given channel layout.
        /// </returns>
        public static bool MixdownHasRemixSupport(HBMixdown mixdown, ulong layout)
        {
            return hbFunctions.hb_mixdown_has_remix_support(mixdown.Id, layout) > 0;
        }

        /// <summary>
        /// Determines if the given encoder supports the given mixdown.
        /// </summary>
        /// <param name="mixdown">
        /// The mixdown to evaluate.
        /// </param>
        /// <param name="encoder">
        /// The encoder to evaluate.
        /// </param>
        /// <returns>
        /// True if the encoder supports the mixdown.
        /// </returns>
        public static bool MixdownHasCodecSupport(HBMixdown mixdown, HBAudioEncoder encoder)
        {
            return hbFunctions.hb_mixdown_has_codec_support(mixdown.Id, (uint)encoder.Id) > 0;
        }

        /// <summary>
        /// Determines if a mixdown is available for a given track and encoder.
        /// </summary>
        /// <param name="mixdown">
        /// The mixdown to evaluate.
        /// </param>
        /// <param name="encoder">
        /// The encoder to evaluate.
        /// </param>
        /// <param name="channelLayout">channel layout of the source track</param>
        /// <returns>True if available.</returns>
        public static bool MixdownIsSupported(HBMixdown mixdown, HBAudioEncoder encoder, long channelLayout)
        {
            return hbFunctions.hb_mixdown_is_supported(mixdown.Id, (uint)encoder.Id, (uint)channelLayout) > 0;
        }

        /// <summary>
        /// Determines if DRC can be applied to the given track with the given encoder.
        /// </summary>
        /// <param name="trackNumber">
        /// The track Number.
        /// </param>
        /// <param name="encoder">
        /// The encoder to use for DRC.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <returns>
        /// True if DRC can be applied to the track with the given encoder.
        /// </returns>
        public static bool CanApplyDrc(IntPtr handle, int trackNumber, HBAudioEncoder encoder, int title)
        {
            return hbFunctions.hb_audio_can_apply_drc2(handle, title, trackNumber, encoder.Id) > 0; 
        }

        /// <summary>
        /// Determines if the given input audio codec can be passed through.
        /// </summary>
        /// <param name="codecId">
        /// The input codec to consider.
        /// </param>
        /// <returns>
        /// True if the codec can be passed through.
        /// </returns>
        public static bool CanPassthroughAudio(int codecId)
        {
            return (codecId & NativeConstants.HB_ACODEC_PASS_MASK) > 0;
        }

        /// <summary>
        /// Sanitizes a mixdown given the output codec and input channel layout.
        /// </summary>
        /// <param name="mixdown">
        /// The desired mixdown.
        /// </param>
        /// <param name="encoder">
        /// The output encoder to be used.
        /// </param>
        /// <param name="layout">
        /// The input channel layout.
        /// </param>
        /// <returns>
        /// A sanitized mixdown value.
        /// </returns>
        public static HBMixdown SanitizeMixdown(HBMixdown mixdown, HBAudioEncoder encoder, ulong layout)
        {
            if (mixdown == null || encoder == null)
            {
                return null;
            }

            int sanitizedMixdown = hbFunctions.hb_mixdown_get_best((uint)encoder.Id, layout, mixdown.Id);
            if (sanitizedMixdown != -1)
            {
                return Mixdowns.Single(m => m.Id == sanitizedMixdown);
            }

            return Mixdowns.FirstOrDefault(); // "none"
        }

        /// <summary>
        /// Gets the default mixdown for the given audio encoder and channel layout.
        /// </summary>
        /// <param name="encoder">
        /// The output codec to be used.
        /// </param>
        /// <param name="layout">
        /// The input channel layout.
        /// </param>
        /// <returns>
        /// The default mixdown for the given codec and channel layout.
        /// </returns>
        public static HBMixdown GetDefaultMixdown(HBAudioEncoder encoder, ulong layout)
        {
            int defaultMixdown = hbFunctions.hb_mixdown_get_default((uint)encoder.Id, layout);
            return Mixdowns.Single(m => m.Id == defaultMixdown);
        }

        /// <summary>
        /// Sanitizes the given sample rate for the given encoder.
        /// </summary>
        /// <param name="encoder">The encoder.</param>
        /// <param name="sampleRate">The sample rate to sanitize.</param>
        /// <returns>The sanitized sample rate.</returns>
        public static int SanitizeSampleRate(HBAudioEncoder encoder, int sampleRate)
        {
            return hbFunctions.hb_audio_samplerate_find_closest(sampleRate, (uint)encoder.Id);
        }

        /// <summary>
        /// Gets the bitrate limits for the given audio codec, sample rate and mixdown.
        /// </summary>
        /// <param name="encoder">
        /// The audio encoder used.
        /// </param>
        /// <param name="sampleRate">
        /// The sample rate used (Hz).
        /// </param>
        /// <param name="mixdown">
        /// The mixdown used.
        /// </param>
        /// <returns>
        /// Limits on the audio bitrate for the given settings.
        /// </returns>
        public static BitrateLimits GetBitrateLimits(HBAudioEncoder encoder, int sampleRate, HBMixdown mixdown)
        {
            int low = 0;
            int high = 0;

            hbFunctions.hb_audio_bitrate_get_limits((uint)encoder.Id, sampleRate, mixdown.Id, ref low, ref high);

            return new BitrateLimits(low, high);
        }

        /// <summary>
        /// Gets the video quality limits for the given video codec.
        /// </summary>
        /// <param name="encoder">
        /// The video encoder to check.
        /// </param>
        /// <returns>
        /// Limits on the video quality for the encoder.
        /// </returns>
        public static VideoQualityLimits GetVideoQualityLimits(HBVideoEncoder encoder)
        {
            float low = 0;
            float high = 0;
            float granularity = 0;
            int direction = 0;

            hbFunctions.hb_video_quality_get_limits((uint)encoder.Id, ref low, ref high, ref granularity, ref direction);

            return new VideoQualityLimits(low, high, granularity, direction == 0);
        }

        /// <summary>
        /// Sanitizes an audio bitrate given the output codec, sample rate and mixdown.
        /// </summary>
        /// <param name="audioBitrate">
        /// The desired audio bitrate.
        /// </param>
        /// <param name="encoder">
        /// The output encoder to be used.
        /// </param>
        /// <param name="sampleRate">
        /// The output sample rate to be used.
        /// </param>
        /// <param name="mixdown">
        /// The mixdown to be used.
        /// </param>
        /// <returns>
        /// A sanitized audio bitrate.
        /// </returns>
        public static int SanitizeAudioBitrate(int audioBitrate, HBAudioEncoder encoder, int sampleRate, HBMixdown mixdown)
        {
            return hbFunctions.hb_audio_bitrate_get_best((uint)encoder.Id, audioBitrate, sampleRate, mixdown.Id);
        }

        /// <summary>
        /// Gets the default audio bitrate for the given parameters.
        /// </summary>
        /// <param name="encoder">
        /// The encoder to use.
        /// </param>
        /// <param name="sampleRate">
        /// The sample rate to use.
        /// </param>
        /// <param name="mixdown">
        /// The mixdown to use.
        /// </param>
        /// <returns>
        /// The default bitrate for these parameters.
        /// </returns>
        public static int GetDefaultBitrate(HBAudioEncoder encoder, int sampleRate, HBMixdown mixdown)
        {
            return hbFunctions.hb_audio_bitrate_get_default((uint)encoder.Id, sampleRate, mixdown.Id);
        }

        /// <summary>
        /// Gets limits on audio quality for a given encoder.
        /// </summary>
        /// <param name="encoderId">
        /// The audio encoder ID.
        /// </param>
        /// <returns>
        /// Limits on the audio quality for the given encoder.
        /// </returns>
        public static RangeLimits GetAudioQualityLimits(int encoderId)
        {
            float low = 0, high = 0, granularity = 0;
            int direction = 0;
            hbFunctions.hb_audio_quality_get_limits((uint)encoderId, ref low, ref high, ref granularity, ref direction);

            return new RangeLimits(direction == 0, granularity, high, low);
        }

        /// <summary>
        /// Gets limits on audio compression for a given encoder.
        /// </summary>
        /// <param name="encoderId">
        /// The audio encoder ID.
        /// </param>
        /// <returns>
        /// Limits on the audio compression for the given encoder.
        /// </returns>
        public static RangeLimits GetAudioCompressionLimits(int encoderId)
        {
            float low = 0, high = 0, granularity = 0;
            int direction = 0;
            hbFunctions.hb_audio_compression_get_limits((uint)encoderId, ref low, ref high, ref granularity, ref direction);

            return new RangeLimits(direction == 0, granularity, high, low);
        }

        /// <summary>
        /// The get default quality.
        /// </summary>
        /// <param name="encoder">
        /// The encoder.
        /// </param>
        /// <returns>
        /// The <see cref="double"/>.
        /// </returns>
        public static double GetDefaultQuality(HBAudioEncoder encoder)
        {
           return hbFunctions.hb_audio_quality_get_default((uint)encoder.Id);
        }

        /// <summary>
        /// The get default audio compression.
        /// </summary>
        /// <param name="encoder">
        /// The encoder.
        /// </param>
        /// <returns>
        /// The <see cref="double"/>.
        /// </returns>
        public static double GetDefaultAudioCompression(HBAudioEncoder encoder)
        {
            return hbFunctions.hb_audio_compression_get_default((uint)encoder.Id);
        }
    }
}
