// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceSubtitleTrack.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A subtitle track from the source video.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    /// <summary>
    /// A subtitle track from the source video.
    /// </summary>
    public class SourceSubtitleTrack
    {
        /// <summary>
        /// Gets or sets the track number (1-based).
        /// </summary>
        public int TrackNumber { get; set; }

        /// <summary>
        /// Gets or sets the format.
        /// </summary>
        public string Format { get; set; }

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

        public string SourceName { get; set; }

        public string Name { get; set; }

        /// <summary>
        /// Gets or sets subtitle attribute information.
        /// </summary>
        public SubtitleAttributes Attributes { get; set; }
    }
}