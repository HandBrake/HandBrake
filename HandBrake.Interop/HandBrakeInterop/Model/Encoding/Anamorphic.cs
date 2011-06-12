using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
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
