// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Color.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The color.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Scan
{
    /// <summary>
    /// The color.
    /// </summary>
    public class Color
    {
        /// <summary>
        /// Gets or sets the matrix.
        /// </summary>
        public int Matrix { get; set; }

        /// <summary>
        /// Gets or sets the primary.
        /// </summary>
        public int Primary { get; set; }

        /// <summary>
        /// Gets or sets the transfer.
        /// </summary>
        public int Transfer { get; set; }
    }
}