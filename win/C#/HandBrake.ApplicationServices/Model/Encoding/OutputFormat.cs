namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    /// <summary>
    /// The Output format.
    /// </summary>
    public enum OutputFormat
    {
        [Description("MP4")]
        Mp4,

        [Description("M4V")]
        M4V,

        [Description("MKV")]
        Mkv
    }
}
