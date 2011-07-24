// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Converters.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Converters type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
	using System;
	using System.Collections.Generic;

	using HandBrake.Interop.HbLib;
	using HandBrake.Interop.Model.Encoding;
	using HandBrake.Interop.SourceData;

	public static class Converters
	{
		private static Dictionary<double, int> vrates = new Dictionary<double, int>
		{
			{5, 5400000},
			{10, 2700000},
			{12, 2250000},
			{15, 1800000},
			{23.976, 1126125},
			{24, 1125000},
			{25, 1080000},
			{29.97, 900900}
		};

		public static int FramerateToVrate(double framerate)
		{
			if (!vrates.ContainsKey(framerate))
			{
				throw new ArgumentException("Framerate not recognized.", "framerate");
			}

			return vrates[framerate];
		}

		public static int MixdownToNative(Mixdown mixdown)
		{
			if (mixdown == Mixdown.Auto)
			{
				throw new ArgumentException("Cannot convert Auto to native.");
			}

			switch (mixdown)
			{
				case Mixdown.DolbyProLogicII:
					return NativeConstants.HB_AMIXDOWN_DOLBYPLII;
				case Mixdown.DolbySurround:
					return NativeConstants.HB_AMIXDOWN_DOLBY;
				case Mixdown.Mono:
					return NativeConstants.HB_AMIXDOWN_MONO;
				case Mixdown.SixChannelDiscrete:
					return NativeConstants.HB_AMIXDOWN_6CH;
				case Mixdown.Stereo:
					return NativeConstants.HB_AMIXDOWN_STEREO;
			}

			return 0;
		}

		public static Mixdown NativeToMixdown(int mixdown)
		{
			switch (mixdown)
			{
				case NativeConstants.HB_AMIXDOWN_MONO:
					return Mixdown.Mono;
				case NativeConstants.HB_AMIXDOWN_STEREO:
					return Mixdown.Stereo;
				case NativeConstants.HB_AMIXDOWN_DOLBY:
					return Mixdown.DolbySurround;
				case NativeConstants.HB_AMIXDOWN_DOLBYPLII:
					return Mixdown.DolbyProLogicII;
				case NativeConstants.HB_AMIXDOWN_6CH:
					return Mixdown.SixChannelDiscrete;
			}

			throw new ArgumentException("Unrecognized mixdown: " + mixdown, "mixdown");
		}

		/// <summary>
		/// Gets the native code for the given encoder.
		/// </summary>
		/// <param name="encoder">The audio encoder to convert. Cannot be AudioEncoder.Passthrough.</param>
		/// <returns>The native code for the encoder.</returns>
		public static uint AudioEncoderToNative(AudioEncoder encoder)
		{
			switch (encoder)
			{
				case AudioEncoder.Ac3Passthrough:
					return NativeConstants.HB_ACODEC_AC3_PASS;
				case AudioEncoder.Faac:
					return NativeConstants.HB_ACODEC_FAAC;
				case AudioEncoder.Lame:
					return NativeConstants.HB_ACODEC_LAME;
				case AudioEncoder.Ac3:
					return NativeConstants.HB_ACODEC_AC3;
				case AudioEncoder.Vorbis:
					return NativeConstants.HB_ACODEC_VORBIS;
			}

			return 0;
		}

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
				default:
					return AudioCodec.Other;
			}
		}
	}
}
