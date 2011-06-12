namespace HandBrake.Interop
{
	using System.ComponentModel.DataAnnotations;

	public enum Anamorphic
	{
		[Display(Name = "None")]
		None = 0,
		[Display(Name = "Strict")]
		Strict,
		[Display(Name = "Loose")]
		Loose,
		[Display(Name = "Custom")]
		Custom
	}
}
