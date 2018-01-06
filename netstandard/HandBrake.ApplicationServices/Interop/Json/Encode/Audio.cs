// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Audio.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Encode
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
        public uint[] CopyMask { get; set; }

        /// <summary>
        /// Gets or sets the fallback encoder.
        /// </summary>
        public int FallbackEncoder { get; set; }
    }
}