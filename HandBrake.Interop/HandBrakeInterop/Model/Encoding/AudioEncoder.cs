// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioEncoder type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
	using System.ComponentModel.DataAnnotations;

	public enum AudioEncoder
	{
		[Display(Name = "AAC (faac)")]
		Faac = 0,

        [Display(Name = "AAC (ffmpeg)")]
        ffaac,

		[Display(Name = "MP3 (lame)")]
		Lame,

		[Display(Name = "AC3 (ffmpeg)")]
		Ac3,

		[Display(Name = "Passthrough")]
		Passthrough,

		[Display(Name = "Passthrough (AC3)")]
		Ac3Passthrough,

        [Display(Name = "DTS Passthru")]
        DtsPassthrough,

        [Display(Name = "DTS-HD Passthru")]
        DtsHDPassthrough,

        [Display(Name = "AAC Passthru")]
        AacPassthru,

        [Display(Name = "MP3 Passthru")]
        Mp3Passthru,

		[Display(Name = "Vorbis (vorbis)")]
		Vorbis
	}
}
