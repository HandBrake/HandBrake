namespace HandBrake.Interop
{
	using System;

	public class EncodeCompletedEventArgs : EventArgs
	{
		public bool Error { get; set; }
	}
}
