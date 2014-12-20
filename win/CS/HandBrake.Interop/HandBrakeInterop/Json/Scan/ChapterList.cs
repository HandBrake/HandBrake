// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChapterList.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The chapter list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Json.Scan
{
    /// <summary>
    /// The chapter list.
    /// </summary>
    internal class ChapterList
    {
        /// <summary>
        /// Gets or sets the duration.
        /// </summary>
        public Duration2 Duration { get; set; }

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        public string Name { get; set; }
    }
}