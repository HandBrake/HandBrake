// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RangeLimits.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The range limits.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    /// <summary>
    /// The range limits.
    /// </summary>
    public class RangeLimits
    {
        #region Properties

        /// <summary>
        /// Gets or sets a value indicating whether ascending.
        /// </summary>
        public bool Ascending { get; set; }

        /// <summary>
        /// Gets or sets the granularity.
        /// </summary>
        public float Granularity { get; set; }

        /// <summary>
        /// Gets or sets the high.
        /// </summary>
        public float High { get; set; }

        /// <summary>
        /// Gets or sets the low.
        /// </summary>
        public float Low { get; set; }

        #endregion
    }
}