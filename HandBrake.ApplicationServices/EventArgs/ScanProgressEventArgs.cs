/*  ScanProgressEventArgs.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.EventArgs
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
    }
}
