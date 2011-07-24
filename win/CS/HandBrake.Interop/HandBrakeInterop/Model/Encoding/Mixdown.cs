// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Mixdown.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Mixdown type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
	using System.ComponentModel.DataAnnotations;

	public enum Mixdown
	{
		[Display(Name = "Dolby Pro Logic II")]
		DolbyProLogicII = 0,

		[Display(Name = "Auto")]
		Auto,

		[Display(Name = "Mono")]
		Mono,

		[Display(Name = "Stereo")]
		Stereo,

		[Display(Name = "Dolby Surround")]
		DolbySurround,

		[Display(Name = "6 Channel Discrete")]
		SixChannelDiscrete
	}
}
