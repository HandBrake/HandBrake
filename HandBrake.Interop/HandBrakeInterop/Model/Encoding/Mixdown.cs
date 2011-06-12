namespace HandBrake.Interop
{
	using System.ComponentModel.DataAnnotations;

	public enum Mixdown
	{
		[Display(Name = "Dolby Pro Logic II")]
		DolbyProLogicII = 0,

		[Display(Name = "Auto")]
		Auto,

		[Display(Name = "Mono")]
		Mono,

		[Display(Name = "Stereo")]
		Stereo,

		[Display(Name = "Dolby Surround")]
		DolbySurround,

		[Display(Name = "6 Channel Discrete")]
		SixChannelDiscrete
	}
}
