// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Scan Progress Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.EventArgs
{
    using System;

    /// <summary>
    /// Scan Progress Event Args
    /// </summary>
    public class ScanProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Gets or sets the title currently being scanned.
        /// </summary>
        public int CurrentTitle { get; set; }

        /// <summary>
        /// Gets or sets the total number of Titles.
        /// </summary>
        public int Titles { get; set; }

        /// <summary>
        /// Gets or sets the percentage.
        /// </summary>
        public decimal Percentage { get; set; }
    }
}
