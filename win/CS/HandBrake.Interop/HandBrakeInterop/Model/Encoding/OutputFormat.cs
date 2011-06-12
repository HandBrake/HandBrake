namespace HandBrake.Interop
{
	using System.ComponentModel.DataAnnotations;

	public enum OutputFormat
	{
		[Display(Name = "MP4")]
		Mp4,
		[Display(Name = "MKV")]
		Mkv
	}
}
