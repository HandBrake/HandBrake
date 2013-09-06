// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
	using System.ComponentModel.DataAnnotations;

	using HandBrake.Interop.Attributes;

    /// <summary>
    /// The video encoder.
    /// </summary>
    public enum VideoEncoder
	{
		[Display(Name = "H.264 (x264)")]
        [ShortName("x264")]
		X264 = 0,

        [Display(Name = "H.264 (Intel QSV)")]
        QuickSync,

		[Display(Name = "MPEG-4 (FFmpeg)")]
        [ShortName("mpeg4")]
		FFMpeg,

		[Display(Name = "MPEG-2 (FFmpeg)")]
        [ShortName("mpeg2")]
		FFMpeg2,

		[Display(Name = "VP3 (Theora)")]
        [ShortName("theora")]
		Theora
	}
}
