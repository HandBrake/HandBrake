// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FrameRate.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The frame rate.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Scan
{
    /// <summary>
    /// The frame rate.
    /// </summary>
    public class FrameRate
    {
        /// <summary>
        /// Gets or sets the den.
        /// </summary>
        public int Den { get; set; }

        /// <summary>
        /// Gets or sets the num.
        /// </summary>
        public int Num { get; set; }
    }
}