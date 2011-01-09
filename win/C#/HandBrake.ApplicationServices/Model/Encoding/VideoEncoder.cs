namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    public enum VideoEncoder
    {
        [Description("H.264 (x264)")]
        X264 = 0,

        [Description("MPEG-4 (FFMpeg)")]
        FFMpeg,

        [Description("VP3 (Theora)")]
        Theora
    }
}
