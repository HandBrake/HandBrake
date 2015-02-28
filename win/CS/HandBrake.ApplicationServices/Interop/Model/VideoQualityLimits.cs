// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoQualityLimits.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoQualityLimits type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model
{
	/// <summary>
	/// Represents limits on video quality for a particular encoder.
	/// </summary>
	public class VideoQualityLimits
	{
		/// <summary>
		/// Gets or sets the inclusive lower limit for the quality.
		/// </summary>
		public float Low { get; set; }

		/// <summary>
		/// Gets or sets the inclusive upper limit for the quality.
		/// </summary>
		public float High { get; set; }

		/// <summary>
		/// Gets or sets the granularity for the quality.
		/// </summary>
		public float Granularity { get; set; }

		/// <summary>
		/// Gets or sets a value indicating whether the quality increases as the number increases.
		/// </summary>
		public bool Ascending { get; set; }
	}
}
