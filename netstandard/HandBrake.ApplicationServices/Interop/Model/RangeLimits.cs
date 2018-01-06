// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RangeLimits.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The range limits.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model
{
    /// <summary>
    /// The range limits.
    /// </summary>
    public class RangeLimits
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RangeLimits"/> class.
        /// </summary>
        /// <param name="ascending">
        /// The ascending.
        /// </param>
        /// <param name="granularity">
        /// The granularity.
        /// </param>
        /// <param name="high">
        /// The high.
        /// </param>
        /// <param name="low">
        /// The low.
        /// </param>
        public RangeLimits(bool @ascending, float granularity, float high, float low)
        {
            this.Ascending = @ascending;
            this.Granularity = granularity;
            this.High = high;
            this.Low = low;
        }

        /// <summary>
        /// Gets a value indicating whether ascending.
        /// </summary>
        public bool Ascending { get; private set; }

        /// <summary>
        /// Gets the granularity.
        /// </summary>
        public float Granularity { get; private set; }

        /// <summary>
        /// Gets the high.
        /// </summary>
        public float High { get; private set; }

        /// <summary>
        /// Gets the low.
        /// </summary>
        public float Low { get; private set; }
    }
}