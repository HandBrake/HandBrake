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
		/// Initializes a new instance of the AudioEncoding class.
		/// </summary>
		public AudioEncoding()
		{
			// Initialize to -1 to differentiate a compression of 0 from uninitialized.
			this.Compression = -1;
		}

		/// <summary>
		/// Gets or sets the chosen track to apply the encoding to.
		/// </summary>
		/// <remarks>1-based index. 0 means apply to all tracks.</remarks>
		public int InputNumber { get; set; }

		/// <summary>
		/// Gets or sets the encoder to use.
		/// </summary>
		public string Encoder { get; set; }

		/// <summary>
		/// Will pass through the track if it maches the codec type.
		/// </summary>
		public bool PassthroughIfPossible { get; set; }

		/// <summary>
		/// Gets or sets the encode rate type (bitrate or quality).
		/// </summary>
		public AudioEncodeRateType EncodeRateType { get; set; }

		/// <summary>
		/// Gets or sets the bitrate (in kbps) of this track.
		/// </summary>
		public int Bitrate { get; set; }

		/// <summary>
		/// Gets or sets the target audio quality for this track.
		/// </summary>
		public float Quality { get; set; }

		/// <summary>
		/// Gets or sets the target audio compression for this track.
		/// </summary>
		public float Compression { get; set; }

		/// <summary>
		/// Gets or sets the mixdown.
		/// </summary>
		public string Mixdown { get; set; }

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
