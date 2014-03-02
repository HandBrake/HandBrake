// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Converters.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Converters type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Converters
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;

    using HandBrake.Interop.HbLib;
    using HandBrake.Interop.Helpers;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Model.Encoding;
    using HandBrake.Interop.SourceData;

    /// <summary>
	/// Converters for various encoding values.
	/// </summary>
	public static class Converters
	{
		/// <summary>
		/// Video Frame Rates
		/// </summary>
		private static readonly Dictionary<double, int> VideoRates;

		/// <summary>
		/// Initializes static members of the Converters class.
		/// </summary>
		static Converters()
		{
			HandBrakeUtils.EnsureGlobalInit();

			VideoRates = new Dictionary<double, int>();
			foreach (var framerate in Encoders.VideoFramerates)
			{
				VideoRates.Add(double.Parse(framerate.Name, CultureInfo.InvariantCulture), framerate.Rate);
			}
		}

		/// <summary>
		/// Convert Framerate to Video Rates
		/// </summary>
		/// <param name="framerate">
		/// The framerate.
		/// </param>
		/// <returns>
		/// The vrate if a valid framerate is passed in.
		/// </returns>
		/// <exception cref="ArgumentException">
		/// Thrown when framerate is invalid.
		/// </exception>
		public static int FramerateToVrate(double framerate)
		{
			if (!VideoRates.ContainsKey(framerate))
			{
				throw new ArgumentException("Framerate not recognized.", "framerate");
			}

			return VideoRates[framerate];
		}

		/// <summary>
		/// Convert Native HB Internal Audio int to a AudioCodec model.
		/// </summary>
		/// <param name="codec">
		/// The codec.
		/// </param>
		/// <returns>
		/// An AudioCodec object.
		/// </returns>
		public static AudioCodec NativeToAudioCodec(uint codec)
		{
			switch (codec)
			{
				case NativeConstants.HB_ACODEC_AC3:
					return AudioCodec.Ac3;
				case NativeConstants.HB_ACODEC_DCA:
					return AudioCodec.Dts;
				case NativeConstants.HB_ACODEC_DCA_HD:
					return AudioCodec.DtsHD;
				case NativeConstants.HB_ACODEC_LAME:
				case NativeConstants.HB_ACODEC_MP3:
					return AudioCodec.Mp3;
				case NativeConstants.HB_ACODEC_FFAAC:
				case NativeConstants.HB_ACODEC_CA_AAC:
				case NativeConstants.HB_ACODEC_CA_HAAC:
				case NativeConstants.HB_ACODEC_FDK_HAAC: // TODO Check this is correct
				case NativeConstants.HB_ACODEC_FDK_AAC: // TODO Check this is correct
					return AudioCodec.Aac;
				case NativeConstants.HB_ACODEC_FFFLAC:
					return AudioCodec.Flac;

				default:
					return AudioCodec.Other;
			}
		}

		/// <summary>
		/// Converts a native HB encoder structure to an Encoder model.
		/// </summary>
		/// <param name="encoder">The structure to convert.</param>
		/// <returns>The converted model.</returns>
		public static HBVideoEncoder NativeToVideoEncoder(hb_encoder_s encoder)
		{
			return new HBVideoEncoder
			{
				Id = encoder.codec,
				ShortName = encoder.short_name,
				DisplayName = encoder.name,
				CompatibleContainers = encoder.muxers
			};
		}

		/// <summary>
		/// Converts a native HB encoder structure to an Encoder model.
		/// </summary>
		/// <param name="encoder">The structure to convert.</param>
		/// <returns>The converted model.</returns>
		public static HBAudioEncoder NativeToAudioEncoder(hb_encoder_s encoder)
		{
			var result = new HBAudioEncoder
			                 {
			                     Id = encoder.codec,
			                     ShortName = encoder.short_name,
			                     DisplayName = encoder.name,
			                     CompatibleContainers = encoder.muxers,
			                     QualityLimits = Encoders.GetAudioQualityLimits(encoder.codec),
			                     DefaultQuality = HBFunctions.hb_audio_quality_get_default((uint)encoder.codec),
			                     CompressionLimits = Encoders.GetAudioCompressionLimits(encoder.codec),
			                     DefaultCompression =
			                     HBFunctions.hb_audio_compression_get_default((uint)encoder.codec)
			                 };

		    return result;
		}

		/// <summary>
		/// Converts a native HB rate structure to an HBRate object.
		/// </summary>
		/// <param name="rate">The structure to convert.</param>
		/// <returns>The converted rate object.</returns>
		public static HBRate NativeToRate(hb_rate_s rate)
		{
			return new HBRate
				{
					Name = rate.name,
					Rate = rate.rate
				};
		}

		/// <summary>
		/// Converts a native HB mixdown structure to a Mixdown model.
		/// </summary>
		/// <param name="mixdown">The structure to convert.</param>
		/// <returns>The converted model.</returns>
		public static HBMixdown NativeToMixdown(hb_mixdown_s mixdown)
		{
			return new HBMixdown
				{
					Id = mixdown.amixdown,
					ShortName = mixdown.short_name,
					DisplayName = mixdown.name
				};
		}

		/// <summary>
		/// Converts a native HB container structure into an HBContainer object.
		/// </summary>
		/// <param name="container">The structure to convert.</param>
		/// <returns>The converted structure.</returns>
		public static HBContainer NativeToContainer(hb_container_s container)
		{
			return new HBContainer
				{
					DisplayName = container.name,
					ShortName = container.short_name,
					DefaultExtension = container.default_extension,
					Id = container.format
				};
		}

		/// <summary>
		/// Converts a native language structure to a Language object.
		/// </summary>
		/// <param name="language">The structure to convert.</param>
		/// <returns>The converted structure.</returns>
		public static Language NativeToLanguage(iso639_lang_t language)
		{
			string englishName = InteropUtilities.ReadUtf8Ptr(language.eng_name);
			string nativeName = InteropUtilities.ReadUtf8Ptr(language.native_name);
			return new Language
				{
					Code = language.iso639_2,
					EnglishName = englishName,
					NativeName = nativeName
				};
		}

		/// <summary>
		/// Converts the PTS amount to a TimeSpan. There may be some accuracy loss here.
		/// </summary>
		/// <param name="pts">The PTS to convert.</param>
		/// <returns>The timespan for it.</returns>
		public static TimeSpan PtsToTimeSpan(ulong pts)
		{
			return TimeSpan.FromTicks((long)((pts * 10000000) / 90000));
		}

		/// <summary>
		/// Converts the PTS amount to seconds.
		/// </summary>
		/// <param name="pts">The PTS to convert.</param>
		/// <returns>The corresponding number of seconds.</returns>
		public static double PtsToSeconds(ulong pts)
		{
			return (double)pts / 90000;
		}
	}
}
