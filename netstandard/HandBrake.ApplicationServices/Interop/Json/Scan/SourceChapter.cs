// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceChapter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The a chapter from a video source.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Scan
{
    /// <summary>
    /// The a chapter from a video source.
    /// </summary>
    public class SourceChapter
    {
        /// <summary>
        /// Gets or sets the duration.
        /// </summary>
        public Duration Duration { get; set; }

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        public string Name { get; set; }
    }
}