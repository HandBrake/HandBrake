// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoding.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioEncoding type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
	using System;

	/// <summary>
	/// The Audio Encoding Model
	/// </summary>
	public class AudioEncoding
	{
		/// <summary>
		/// Gets or sets InputNumber.
		/// </summary>
		public int InputNumber { get; set; }

		/// <summary>
		/// Gets or sets Encoder.
		/// </summary>
		public AudioEncoder Encoder { get; set; }

		/// <summary>
		/// Gets or sets the bitrate (in kbps) of this track.
		/// </summary>
		public int Bitrate { get; set; }

		/// <summary>
		/// Gets or sets Mixdown.
		/// </summary>
		public Mixdown Mixdown { get; set; }

		/// <summary>
		/// Gets or sets the sample rate. Obsolete. Use SampleRateRaw instead.
		/// </summary>
		[Obsolete("This property is ignored and only exists for backwards compatibility. Use SampleRateRaw instead.")]
		public string SampleRate { get; set; }

		/// <summary>
		/// Gets or sets the sample rate in Hz.
		/// </summary>
		public int SampleRateRaw { get; set; }

		/// <summary>
		/// Gets or sets Gain.
		/// </summary>
		public int Gain { get; set; }

		/// <summary>
		/// Gets or sets Drc.
		/// </summary>
		public double Drc { get; set; }

		/// <summary>
		/// Gets or sets Name.
		/// </summary>
		public string Name { get; set; }
	}
}
