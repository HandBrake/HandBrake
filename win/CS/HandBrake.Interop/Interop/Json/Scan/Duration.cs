// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Duration.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The duration.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    /// <summary>
    /// The duration.
    /// </summary>
    public class Duration
    {
        /// <summary>
        /// Gets or sets the hours.
        /// </summary>
        public int Hours { get; set; }

        /// <summary>
        /// Gets or sets the minutes.
        /// </summary>
        public int Minutes { get; set; }

        /// <summary>
        /// Gets or sets the seconds.
        /// </summary>
        public int Seconds { get; set; }

        /// <summary>
        /// Gets or sets the ticks.
        /// </summary>
        public long Ticks { get; set; }
    }
}