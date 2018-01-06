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
        /// Initializes a new instance of the <see cref="VideoQualityLimits"/> class.
        /// </summary>
        /// <param name="low">
        /// The low.
        /// </param>
        /// <param name="high">
        /// The high.
        /// </param>
        /// <param name="granularity">
        /// The granularity.
        /// </param>
        /// <param name="ascending">
        /// The ascending.
        /// </param>
        public VideoQualityLimits(float low, float high, float granularity, bool @ascending)
        {
            this.Low = low;
            this.High = high;
            this.Granularity = granularity;
            this.Ascending = @ascending;
        }

        /// <summary>
        /// Gets the inclusive lower limit for the quality.
        /// </summary>
        public float Low { get; private set; }

        /// <summary>
        /// Gets the inclusive upper limit for the quality.
        /// </summary>
        public float High { get; private set; }

        /// <summary>
        /// Gets the granularity for the quality.
        /// </summary>
        public float Granularity { get; private set; }

        /// <summary>
        /// Gets a value indicating whether the quality increases as the number increases.
        /// </summary>
        public bool Ascending { get; private set; }
    }
}
