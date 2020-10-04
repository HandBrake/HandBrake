// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Audio.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Encode
{
    using System.Collections.Generic;

    /// <summary>
    /// The audio.
    /// </summary>
    public class Audio
    {
        /// <summary>
        /// Gets or sets the audio list.
        /// </summary>
        public List<AudioTrack> AudioList { get; set; }

        /// <summary>
        /// Gets or sets the copy mask.
        /// </summary>
        public string[] CopyMask { get; set; }

        /// <summary>
        /// Gets or sets the fallback encoder.
        /// </summary>
        public string FallbackEncoder { get; set; }
    }
}