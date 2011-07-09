// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoRangeType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoRangeType type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    using System.ComponentModel.DataAnnotations;

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
