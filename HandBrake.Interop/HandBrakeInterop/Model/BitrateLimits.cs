// --------------------------------------------------------------------------------------------------------------------
// <copyright file="BitrateLimits.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the BitrateLimits type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    /// <summary>
    /// The bitrate limits.
    /// </summary>
    public class BitrateLimits
	{
        /// <summary>
        /// Gets or sets the low.
        /// </summary>
        public int Low { get; set; }

        /// <summary>
        /// Gets or sets the high.
        /// </summary>
        public int High { get; set; }
	}
}
