using HandBrakeWPF.Properties;

namespace HandBrakeWPF.Model.Audio
{
    using HandBrake.Interop.Attributes;

    public enum AudioTrackNamingBehaviour
    {

        [DisplayName(typeof(Resources), "AudioTrackNaming_None")]
        [ShortName("none")]
        None,

        [DisplayName(typeof(Resources), "AudioTrackNaming_Unnamed")]
        [ShortName("unnamed")]
        Unnamed,

        [DisplayName(typeof(Resources), "AudioTrackNaming_All")]
        [ShortName("All")]
        All,
    }
}
