// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeCompletedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeCompletedEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
	using System;

	public class EncodeCompletedEventArgs : EventArgs
	{
		public bool Error { get; set; }
	}
}
