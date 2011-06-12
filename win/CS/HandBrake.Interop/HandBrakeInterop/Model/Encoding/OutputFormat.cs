using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Text;
using System.ComponentModel;

namespace HandBrake.Interop
{
	public enum OutputFormat
	{
		[Display(Name = "MP4")]
		Mp4,
		[Display(Name = "MKV")]
		Mkv
	}
}
