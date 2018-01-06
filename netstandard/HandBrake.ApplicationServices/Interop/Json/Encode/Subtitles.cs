// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Subtitles.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The subtitle.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Encode
{
    using System.Collections.Generic;

    /// <summary>
    /// The subtitle.
    /// </summary>
    public class Subtitles
    {
        /// <summary>
        /// Gets or sets the search.
        /// </summary>
        public SubtitleSearch Search { get; set; }

        /// <summary>
        /// Gets or sets the subtitle list.
        /// </summary>
        public List<SubtitleTrack> SubtitleList { get; set; }
    }
}