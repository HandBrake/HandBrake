namespace HandBrake.Interop.Model.Encoding
{
    public enum VideoEncoder
    {
        [DisplayString("H.264 (x264)")]
        X264 = 0,

        [DisplayString("MPEG-4 (FFMpeg)")]
        FFMpeg,

        [DisplayString("VP3 (Theora)")]
        Theora
    }
}
