// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Encoders.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The encoders.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
	using System;
	using System.Collections.Generic;
	using System.Linq;

	using HandBrake.Interop.HbLib;
	using HandBrake.Interop.Model.Encoding;
	using HandBrake.Interop.SourceData;

	/// <summary>
	/// The encoders.
	/// </summary>
	public static class Encoders
	{
		/// <summary>
		/// The audio encoders.
		/// </summary>
		private static List<HBAudioEncoder> audioEncoders;

		/// <summary>
		/// The video encoders.
		/// </summary>
		private static List<HBVideoEncoder> videoEncoders;

		/// <summary>
		/// Video framerates in pts.
		/// </summary>
		private static List<HBRate> videoFramerates; 

		/// <summary>
		/// The mixdowns.
		/// </summary>
		private static List<HBMixdown> mixdowns;

		/// <summary>
		/// The audio bitrates.
		/// </summary>
		private static List<int> audioBitrates;

		/// <summary>
		/// Audio sample rates in Hz.
		/// </summary>
		private static List<HBRate> audioSampleRates; 

		/// <summary>
		/// Initializes static members of the Encoders class.
		/// </summary>
		static Encoders()
		{
			HandBrakeUtils.EnsureGlobalInit();
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
					audioEncoders = InteropUtilities.GetListFromIterator<hb_encoder_s, HBAudioEncoder>(HBFunctions.hb_audio_encoder_get_next, Converters.NativeToAudioEncoder);
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
					videoEncoders = InteropUtilities.GetListFromIterator<hb_encoder_s, HBVideoEncoder>(HBFunctions.hb_video_encoder_get_next, Converters.NativeToVideoEncoder);
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
					videoFramerates = InteropUtilities.GetListFromIterator<hb_rate_s, HBRate>(HBFunctions.hb_video_framerate_get_next, Converters.NativeToRate);
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
					mixdowns = InteropUtilities.GetListFromIterator<hb_mixdown_s, HBMixdown>(HBFunctions.hb_mixdown_get_next, Converters.NativeToMixdown);
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
					audioBitrates = InteropUtilities.GetListFromIterator<hb_rate_s, int>(HBFunctions.hb_audio_bitrate_get_next, b => b.rate);
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
					audioSampleRates = InteropUtilities.GetListFromIterator<hb_rate_s, HBRate>(HBFunctions.hb_audio_samplerate_get_next, Converters.NativeToRate);
				}

				return audioSampleRates;
			}
		} 

		/// <summary>
		/// Gets the audio encoder with the specified short name.
		/// </summary>
		/// <param name="shortName">The name of the audio encoder.</param>
		/// <returns>The requested audio encoder.</returns>
		public static HBAudioEncoder GetAudioEncoder(string shortName)
		{
			return AudioEncoders.SingleOrDefault(e => e.ShortName == shortName);
		}

		/// <summary>
		/// Gets the video encoder with the specified short name.
		/// </summary>
		/// <param name="shortName">The name of the video encoder.</param>
		/// <returns>The requested video encoder.</returns>
		public static HBVideoEncoder GetVideoEncoder(string shortName)
		{
			return VideoEncoders.SingleOrDefault(e => e.ShortName == shortName);
		}

		/// <summary>
		/// Gets the mixdown with the specified short name.
		/// </summary>
		/// <param name="shortName">The name of the mixdown.</param>
		/// <returns>The requested mixdown.</returns>
		public static HBMixdown GetMixdown(string shortName)
		{
			return Mixdowns.SingleOrDefault(m => m.ShortName == shortName);
		}

		/// <summary>
		/// Determines if the given encoder is compatible with the given track.
		/// </summary>
		/// <param name="track">The audio track to examine.</param>
		/// <param name="encoder">The encoder to examine.</param>
		/// <returns>True if the given encoder is comatible with the given audio track.</returns>
		/// <remarks>Only works with passthrough encoders.</remarks>
		public static bool AudioEncoderIsCompatible(AudioTrack track, HBAudioEncoder encoder)
		{
			return (track.CodecId & encoder.Id) > 0;
		}

		/// <summary>
		/// Determines if the given mixdown supports the given channel layout.
		/// </summary>
		/// <param name="mixdown">The mixdown to evaluate.</param>
		/// <param name="layout">The channel layout to evaluate.</param>
		/// <returns>True if the mixdown supports the given channel layout.</returns>
		public static bool MixdownHasRemixSupport(HBMixdown mixdown, ulong layout)
		{
			return HBFunctions.hb_mixdown_has_remix_support(mixdown.Id, layout) > 0;
		}

		/// <summary>
		/// Determines if the given encoder supports the given mixdown.
		/// </summary>
		/// <param name="mixdown">The mixdown to evaluate.</param>
		/// <param name="encoder">The encoder to evaluate.</param>
		/// <returns>True if the encoder supports the mixdown.</returns>
		public static bool MixdownHasCodecSupport(HBMixdown mixdown, HBAudioEncoder encoder)
		{
			return HBFunctions.hb_mixdown_has_codec_support(mixdown.Id, (uint) encoder.Id) > 0;
		}

		/// <summary>
		/// Sanitizes a mixdown given the output codec and input channel layout.
		/// </summary>
		/// <param name="mixdown">The desired mixdown.</param>
		/// <param name="encoder">The output encoder to be used.</param>
		/// <param name="layout">The input channel layout.</param>
		/// <returns>A sanitized mixdown value.</returns>
		public static HBMixdown SanitizeMixdown(HBMixdown mixdown, HBAudioEncoder encoder, ulong layout)
		{
			int sanitizedMixdown = HBFunctions.hb_mixdown_get_best((uint)encoder.Id, layout, mixdown.Id);
			return Mixdowns.Single(m => m.Id == sanitizedMixdown);
		}

		/// <summary>
		/// Gets the default mixdown for the given audio encoder and channel layout.
		/// </summary>
		/// <param name="encoder">The output codec to be used.</param>
		/// <param name="layout">The input channel layout.</param>
		/// <returns>The default mixdown for the given codec and channel layout.</returns>
		public static HBMixdown GetDefaultMixdown(HBAudioEncoder encoder, ulong layout)
		{
			int defaultMixdown = HBFunctions.hb_mixdown_get_default((uint)encoder.Id, layout);
			return Mixdowns.Single(m => m.Id == defaultMixdown);
		}

		/// <summary>
		/// Gets the bitrate limits for the given audio codec, sample rate and mixdown.
		/// </summary>
		/// <param name="encoder">The audio encoder used.</param>
		/// <param name="sampleRate">The sample rate used (Hz).</param>
		/// <param name="mixdown">The mixdown used.</param>
		/// <returns>Limits on the audio bitrate for the given settings.</returns>
		public static BitrateLimits GetBitrateLimits(HBAudioEncoder encoder, int sampleRate, HBMixdown mixdown)
		{
			int low = 0;
			int high = 0;

			HBFunctions.hb_audio_bitrate_get_limits((uint)encoder.Id, sampleRate, mixdown.Id, ref low, ref high);

			return new BitrateLimits { Low = low, High = high };
		}

		/// <summary>
		/// Sanitizes an audio bitrate given the output codec, sample rate and mixdown.
		/// </summary>
		/// <param name="audioBitrate">The desired audio bitrate.</param>
		/// <param name="encoder">The output encoder to be used.</param>
		/// <param name="sampleRate">The output sample rate to be used.</param>
		/// <param name="mixdown">The mixdown to be used.</param>
		/// <returns>A sanitized audio bitrate.</returns>
		public static int SanitizeAudioBitrate(int audioBitrate, HBAudioEncoder encoder, int sampleRate, HBMixdown mixdown)
		{
			return HBFunctions.hb_audio_bitrate_get_best((uint)encoder.Id, audioBitrate, sampleRate, mixdown.Id);
		}

		/// <summary>
		/// Gets the default audio bitrate for the given parameters.
		/// </summary>
		/// <param name="encoder">The encoder to use.</param>
		/// <param name="sampleRate">The sample rate to use.</param>
		/// <param name="mixdown">The mixdown to use.</param>
		/// <returns>The default bitrate for these parameters.</returns>
		public static int GetDefaultBitrate(HBAudioEncoder encoder, int sampleRate, HBMixdown mixdown)
		{
			return HBFunctions.hb_audio_bitrate_get_default((uint) encoder.Id, sampleRate, mixdown.Id);
		}

		/// <summary>
		/// Gets limits on audio quality for a given encoder.
		/// </summary>
		/// <param name="encoderId">The audio encoder ID.</param>
		/// <returns>Limits on the audio quality for the given encoder.</returns>
		internal static RangeLimits GetAudioQualityLimits(int encoderId)
		{
			float low = 0, high = 0, granularity = 0;
			int direction = 0;
			HBFunctions.hb_audio_quality_get_limits((uint)encoderId, ref low, ref high, ref granularity, ref direction);

			return new RangeLimits
			{
				Low = low,
				High = high,
				Granularity = granularity,
				Ascending = direction == 0
			};
		}

		/// <summary>
		/// Gets limits on audio compression for a given encoder.
		/// </summary>
		/// <param name="encoderId">The audio encoder ID.</param>
		/// <returns>Limits on the audio compression for the given encoder.</returns>
		internal static RangeLimits GetAudioCompressionLimits(int encoderId)
		{
			float low = 0, high = 0, granularity = 0;
			int direction = 0;
			HBFunctions.hb_audio_compression_get_limits((uint)encoderId, ref low, ref high, ref granularity, ref direction);

			return new RangeLimits
			{
				Low = low,
				High = high,
				Granularity = granularity,
				Ascending = direction == 0
			};
		}
	}
}
