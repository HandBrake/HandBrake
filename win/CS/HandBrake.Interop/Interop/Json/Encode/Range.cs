// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Range.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The range.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Encode
{
    /// <summary>
    /// The range.
    /// </summary>
    public class Range
    {
        /// <summary>
        /// Gets or sets the chapter end.
        /// Type is "chapter", "time", "frame", or "preview".
        /// </summary>
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the chapter start.
        /// </summary>
        public long? Start { get; set; }

        /// <summary>
        /// Gets or sets the frame to start.
        /// </summary>
        public long? End { get; set; }

        /// <summary>
        /// Gets or sets the seek points.
        /// </summary>
        public long? SeekPoints { get; set; }
    }
}