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
		/// Gets or sets the total progress fraction for the scan.
		/// </summary>
		public float Progress { get; set; }

		/// <summary>
		/// Gets or sets the current preview being processed on the scan.
		/// </summary>
		public int CurrentPreview { get; set; }

		/// <summary>
		/// Gets or sets the total number of previews to process.
		/// </summary>
		public int Previews { get; set; }

		/// <summary>
		/// Gets or sets the current title being processed on the scan.
		/// </summary>
		public int CurrentTitle { get; set; }

		/// <summary>
		/// Gets or sets the total number of titles to process.
		/// </summary>
		public int Titles { get; set; }
	}
}
