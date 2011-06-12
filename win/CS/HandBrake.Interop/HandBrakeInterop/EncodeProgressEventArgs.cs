using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
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
