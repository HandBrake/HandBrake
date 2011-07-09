// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoEncoder type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;

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
