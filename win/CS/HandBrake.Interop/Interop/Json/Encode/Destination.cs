// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Destination.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The destination.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Encode
{
    using System.Collections.Generic;

    /// <summary>
    /// The destination.
    /// </summary>
    public class Destination
    {
        /// <summary>
        /// Gets or sets the chapter list.
        /// </summary>
        public List<Chapter> ChapterList { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether chapter markers.
        /// </summary>
        public bool ChapterMarkers { get; set; }

        /// <summary>
        /// Use Legacy A/V Alignment rather than Edit Lists.
        /// </summary>
        public bool AlignAVStart { get; set; }

        /// <summary>
        /// Gets or sets the file.
        /// </summary>
        public string File { get; set; }

        /// <summary>
        /// Gets or sets the options.
        /// </summary>
        public Options Options { get; set; }

        /// <summary>
        /// Gets or sets the mux.
        /// </summary>
        public string Mux { get; set; }
    }
}