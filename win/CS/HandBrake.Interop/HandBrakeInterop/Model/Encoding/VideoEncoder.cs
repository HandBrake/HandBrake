using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
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
