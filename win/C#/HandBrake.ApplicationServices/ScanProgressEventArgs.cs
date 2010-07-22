/*  ScanProgressEventArgs.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices
{
    using System;

    /// <summary>
    /// Scan Progress Event Args
    /// </summary>
    public class ScanProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Gets or sets TotalTitles.
        /// </summary>
        public int TotalTitles { get; set; }

        /// <summary>
        /// Gets or sets CurrentTitle.
        /// </summary>
        public int CurrentTitle { get; set; }
    }
}
