// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Range.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The range.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Json.Encode
{
    /// <summary>
    /// The range.
    /// </summary>
    public class Range
    {
        /// <summary>
        /// Gets or sets the chapter end.
        /// </summary>
        public int? ChapterEnd { get; set; }

        /// <summary>
        /// Gets or sets the chapter start.
        /// </summary>
        public int? ChapterStart { get; set; }

        /// <summary>
        /// Gets or sets the frame to start.
        /// </summary>
        public int? FrameToStart { get; set; }

        /// <summary>
        /// Gets or sets the frame to stop.
        /// </summary>
        public int? FrameToStop { get; set; }

        /// <summary>
        /// Gets or sets the pts to start.
        /// </summary>
        public int? PtsToStart { get; set; }

        /// <summary>
        /// Gets or sets the pts to stop.
        /// </summary>
        public int? PtsToStop { get; set; }

        /// <summary>
        /// Gets or sets the start at preview.
        /// </summary>
        public int? StartAtPreview { get; set; }

        /// <summary>
        /// Gets or sets the seek points.
        /// </summary>
        public int? SeekPoints { get; set; }
    }
}