// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QSV.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The qsv.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Encode
{
    /// <summary>
    /// The qsv.
    /// </summary>
    public class QSV
    {
        /// <summary>
        /// Gets or sets a value indicating whether decode.
        /// </summary>
        public bool Decode { get; set; }

        /// <summary>
        /// Gets or sets the async depth.
        /// </summary>
        public int AsyncDepth { get; set; }
    }
}
