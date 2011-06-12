namespace HandBrake.Interop
{
	using System.ComponentModel.DataAnnotations;

	public enum VideoEncoder
	{
		[Display(Name = "H.264 (x264)")]
		X264 = 0,

		[Display(Name = "MPEG-4 (FFMpeg)")]
		FFMpeg,

		[Display(Name = "VP3 (Theora)")]
		Theora
	}
}
