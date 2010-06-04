// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ScanProgressEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    using System;

    public class ScanProgressEventArgs : EventArgs
    {
        public int CurrentTitle { get; set; }
        public int Titles { get; set; }
    }
}
