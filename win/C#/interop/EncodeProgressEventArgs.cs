namespace HandBrake.Interop
{
    using System;

    public class EncodeProgressEventArgs : EventArgs
    {
        public float FractionComplete { get; set; }
        public float CurrentFrameRate { get; set; }
        public float AverageFrameRate { get; set; }
        public TimeSpan EstimatedTimeLeft { get; set; }
        public int Pass { get; set; }
    }
}
