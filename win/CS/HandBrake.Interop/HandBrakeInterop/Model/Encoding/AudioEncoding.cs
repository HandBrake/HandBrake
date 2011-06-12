namespace HandBrake.Interop
{
	using System;

	public class AudioEncoding
	{
		public int InputNumber { get; set; }
		public AudioEncoder Encoder { get; set; }

		/// <summary>
		/// Gets or sets the bitrate (in kbps) of this track.
		/// </summary>
		public int Bitrate { get; set; }

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

		public int Gain { get; set; }
		public double Drc { get; set; }
		public string Name { get; set; }
	}
}
