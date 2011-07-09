// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeProgressEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeProgressEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
	using System;

	public class EncodeProgressEventArgs : EventArgs
	{
		public float FractionComplete { get; set; }
		public float CurrentFrameRate { get; set; }
		public float AverageFrameRate { get; set; }
		public TimeSpan EstimatedTimeLeft { get; set; }

		/// <summary>
		/// Gets or sets the current encoding pass. (-1: subtitle scan, 1: first pass, 2: second pass)
		/// </summary>
		public int Pass { get; set; }
	}
}
