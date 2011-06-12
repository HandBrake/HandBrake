namespace HandBrake.SourceData
{
	using System.ComponentModel.DataAnnotations;

	public enum InputType
	{
		[Display(Name = "File")]
		Stream,

		[Display(Name = "DVD")]
		Dvd,

		[Display(Name = "Blu-ray")]
		Bluray
	}
}
