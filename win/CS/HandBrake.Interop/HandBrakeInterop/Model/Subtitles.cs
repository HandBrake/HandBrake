// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Subtitles.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Subtitles type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    using System.Collections.Generic;

    /// <summary>
    /// The subtitles.
    /// </summary>
    public class Subtitles
    {
        #region Properties

        /// <summary>
        /// Gets or sets the source subtitles.
        /// </summary>
        public List<SourceSubtitle> SourceSubtitles { get; set; }

        /// <summary>
        /// Gets or sets the srt subtitles.
        /// </summary>
        public List<SrtSubtitle> SrtSubtitles { get; set; }

        #endregion
    }
}