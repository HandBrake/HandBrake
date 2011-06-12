namespace HandBrake.Interop
{
	using System.ComponentModel.DataAnnotations;

	public enum VideoRangeType
	{
		[Display(Name = "Chapters")]
		Chapters,

		[Display(Name = "Seconds")]
		Seconds,

		[Display(Name = "Frames")]
		Frames
	}
}
