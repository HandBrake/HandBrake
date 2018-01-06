// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Scanning.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The scanning.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.State
{
    /// <summary>
    /// The scanning.
    /// </summary>
    public class Scanning
    {
        /// <summary>
        /// Gets or sets the preview.
        /// </summary>
        public int Preview { get; set; }

        /// <summary>
        /// Gets or sets the preview count.
        /// </summary>
        public int PreviewCount { get; set; }

        /// <summary>
        /// Gets or sets the progress.
        /// </summary>
        public double Progress { get; set; }

        /// <summary>
        /// Gets or sets the title.
        /// </summary>
        public int Title { get; set; }

        /// <summary>
        /// Gets or sets the title count.
        /// </summary>
        public int TitleCount { get; set; }
    }
}