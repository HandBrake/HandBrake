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

	/// <summary>
	/// The video range type.
	/// </summary>
	public enum VideoRangeType
	{
		/// <summary>
		/// The entire title.
		/// </summary>
		[Display(Name = "All")]
		All,

		/// <summary>
		/// A chapter range.
		/// </summary>
		[Display(Name = "Chapters")]
		Chapters, 

		/// <summary>
		/// A timespan range in seconds.
		/// </summary>
		[Display(Name = "Seconds")]
		Seconds, 

		/// <summary>
		/// A frame range.
		/// </summary>
		[Display(Name = "Frames")]
		Frames
	}
}