// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleSearch.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The search.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Encode
{
    /// <summary>
    /// The search.
    /// </summary>
    public class SubtitleSearch
    {
        /// <summary>
        /// Gets or sets a value indicating whether burn.
        /// </summary>
        public bool Burn { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether default.
        /// </summary>
        public bool Default { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether enable.
        /// </summary>
        public bool Enable { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether forced.
        /// </summary>
        public bool Forced { get; set; }
    }
}