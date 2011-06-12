using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel.DataAnnotations;

namespace HandBrake.SourceData
{
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
