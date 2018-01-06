// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ScanProgressEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.EventArgs
{
    using System;

    /// <summary>
    /// The Scan Progress Event Args
    /// </summary>
    public class ScanProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ScanProgressEventArgs"/> class.
        /// </summary>
        /// <param name="progress">
        /// The progress.
        /// </param>
        /// <param name="currentPreview">
        /// The current preview.
        /// </param>
        /// <param name="previews">
        /// The previews.
        /// </param>
        /// <param name="currentTitle">
        /// The current title.
        /// </param>
        /// <param name="titles">
        /// The titles.
        /// </param>
        public ScanProgressEventArgs(double progress, int currentPreview, int previews, int currentTitle, int titles)
        {
            this.Progress = progress;
            this.CurrentPreview = currentPreview;
            this.Previews = previews;
            this.CurrentTitle = currentTitle;
            this.Titles = titles;
        }

        /// <summary>
        /// Gets the total progress fraction for the scan.
        /// </summary>
        public double Progress { get; private set; }

        /// <summary>
        /// Gets the current preview being processed on the scan.
        /// </summary>
        public int CurrentPreview { get; private set; }

        /// <summary>
        /// Gets the total number of previews to process.
        /// </summary>
        public int Previews { get; private set; }

        /// <summary>
        /// Gets the current title being processed on the scan.
        /// </summary>
        public int CurrentTitle { get; private set; }

        /// <summary>
        /// Gets the total number of titles to process.
        /// </summary>
        public int Titles { get; private set; }
    }
}
