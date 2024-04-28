// --------------------------------------------------------------------------------------------------------------------
// <copyright file="JsonEncodeObject.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The root object.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Encode
{
    using System.Collections.Generic;

    using HandBrake.Interop.Interop.Json.Shared;

    /// <summary>
    /// The root object.
    /// </summary>
    public class JsonEncodeObject
    {
        /// <summary>
        /// Gets or sets the audio.
        /// </summary>
        public Audio Audio { get; set; }

        /// <summary>
        /// Gets or sets the destination.
        /// </summary>
        public Destination Destination { get; set; }

        /// <summary>
        /// Gets or sets the filter.
        /// </summary>
        public Filters Filters { get; set; }

        /// <summary>
        /// Gets or sets the PAR
        /// </summary>
        public PAR PAR { get; set; }

        /// <summary>
        /// Gets or sets the meta data.
        /// </summary>
        public Dictionary<string, string> Metadata { get; set; }

        /// <summary>
        /// Gets or sets the sequence id.
        /// </summary>
        public int SequenceID { get; set; }

        /// <summary>
        /// Gets or sets the source.
        /// </summary>
        public Source Source { get; set; }

        /// <summary>
        /// Gets or sets the subtitle.
        /// </summary>
        public Subtitles Subtitle { get; set; }

        /// <summary>
        /// Gets or sets the video.
        /// </summary>
        public Video Video { get; set; }
    }
}