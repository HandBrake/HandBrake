namespace HandBrake.Interop
{
    using System;

    public class ScanProgressEventArgs : EventArgs
    {
        public int CurrentTitle { get; set; }
        public int Titles { get; set; }
    }
}
