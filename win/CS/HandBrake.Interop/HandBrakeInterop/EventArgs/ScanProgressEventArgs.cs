// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScanProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ScanProgressEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.EventArgs
{
    using System;

    /// <summary>
    /// The Scan Progress Event Args
    /// </summary>
    public class ScanProgressEventArgs : EventArgs
	{
	    /// <summary>
	    /// Gets or sets CurrentTitle.
	    /// </summary>
	    public int CurrentTitle { get; set; }

	    /// <summary>
	    /// Gets or sets Titles.
	    /// </summary>
	    public int Titles { get; set; }
	}
}
