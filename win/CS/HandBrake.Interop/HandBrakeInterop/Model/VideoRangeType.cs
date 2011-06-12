using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel.DataAnnotations;

namespace HandBrake.Interop
{
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
