namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    /// <summary>
    /// The Mixdown Mode
    /// </summary>
    public enum Mixdown
    {
        [Description("Dolby Pro Logic II")]
        DolbyProLogicII = 0,

        [Description("Auto")]
        Auto,

        [Description("Mono")]
        Mono,

        [Description("Stereo")]
        Stereo,

        [Description("Dolby Surround")]
        DolbySurround,

        [Description("6 Channel Discrete")]
        SixChannelDiscrete
    }
}
