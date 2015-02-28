// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Duration2.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The duration 2.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Scan
{
    /// <summary>
    /// The duration 2.
    /// </summary>
    internal class Duration2
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
        public int Ticks { get; set; }
    }
}