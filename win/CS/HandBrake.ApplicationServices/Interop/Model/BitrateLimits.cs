// --------------------------------------------------------------------------------------------------------------------
// <copyright file="BitrateLimits.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the BitrateLimits type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model
{
    /// <summary>
    /// Represents bitrate limits as a range.
    /// </summary>
    public class BitrateLimits
    {
        /// <summary>
        /// Gets or sets the inclusive lower limit for the bitrate.
        /// </summary>
        public int Low { get; set; }

        /// <summary>
        /// Gets or sets the inclusive upper limit for the bitrate.
        /// </summary>
        public int High { get; set; }
    }
}
