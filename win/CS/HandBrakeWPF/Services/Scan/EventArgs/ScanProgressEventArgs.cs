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
    using System.Runtime.Serialization;

    /// <summary>
    /// Scan Progress Event Args
    /// </summary>
    [DataContract]
    public class ScanProgressEventArgs : EventArgs
    {
        /// <summary>
        /// Gets or sets the title currently being scanned.
        /// </summary>
        [DataMember]
        public int CurrentTitle { get; set; }

        /// <summary>
        /// Gets or sets the total number of Titles.
        /// </summary>
        [DataMember]
        public int Titles { get; set; }

        /// <summary>
        /// Gets or sets the percentage.
        /// </summary>
        [DataMember]
        public decimal Percentage { get; set; }
    }
}
