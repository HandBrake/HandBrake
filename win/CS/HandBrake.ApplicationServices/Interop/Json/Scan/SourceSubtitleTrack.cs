// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceSubtitleTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The subtitle list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Scan
{
    /// <summary>
    /// The subtitle list.
    /// </summary>
    public class SourceSubtitleTrack
    {
        /// <summary>
        /// Gets or sets the format.
        /// </summary>
        public int Format { get; set; }

        /// <summary>
        /// Gets or sets the language.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the language code.
        /// </summary>
        public string LanguageCode { get; set; }

        /// <summary>
        /// Gets or sets the source.
        /// </summary>
        public int Source { get; set; }
    }
}