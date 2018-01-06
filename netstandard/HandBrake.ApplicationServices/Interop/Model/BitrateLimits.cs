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
        /// Initializes a new instance of the <see cref="BitrateLimits"/> class.
        /// </summary>
        /// <param name="low">
        /// The low.
        /// </param>
        /// <param name="high">
        /// The high.
        /// </param>
        public BitrateLimits(int low, int high)
        {
            this.Low = low;
            this.High = high;
        }

        /// <summary>
        /// Gets the inclusive lower limit for the bitrate.
        /// </summary>
        public int Low { get; private set; }

        /// <summary>
        /// Gets the inclusive upper limit for the bitrate.
        /// </summary>
        public int High { get; private set; }
    }
}
