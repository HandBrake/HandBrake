using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
	public enum AudioEncoder
	{
		[Display(Name = "AAC (faac)")]
		Faac = 0,

		[Display(Name = "MP3 (lame)")]
		Lame,

		[Display(Name = "AC3 (ffmpeg)")]
		Ac3,

		[Display(Name = "Passthrough (AC3/DTS)")]
		Passthrough,

		[Display(Name = "Passthrough (AC3)")]
		Ac3Passthrough,

		[Display(Name = "Passthrough (DTS)")]
		DtsPassthrough,

		[Display(Name = "Vorbis (vorbis)")]
		Vorbis
	}
}
