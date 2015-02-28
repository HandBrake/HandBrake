// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeCompletedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeCompletedEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.EventArgs
{
    using System;

    /// <summary>
    /// Encode Completed Event Args
    /// </summary>
    public class EncodeCompletedEventArgs : EventArgs
	{
	    /// <summary>
	    /// Gets or sets a value indicating whether an error occurred during the encode.
	    /// </summary>
	    public bool Error { get; set; }
	}
}
