// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Represents a subtitle track to encode.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Encode
{
    /// <summary>
    /// Represents a subtitle track to encode.
    /// </summary>
    public class SubtitleTrack
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
        /// Gets or sets a value indicating whether force.
        /// </summary>
        public bool Forced { get; set; }

        /// <summary>
        /// Gets or sets the id.
        /// </summary>
        public int ID { get; set; }

        /// <summary>
        /// Gets or sets the offset.
        /// </summary>
        public int Offset { get; set; }

        /// <summary>
        /// Gets or sets the track.
        /// </summary>
        public int Track { get; set; }

        /// <summary>
        /// Gets or sets the srt.
        /// </summary>
        public SRT SRT { get; set; }
    }
}