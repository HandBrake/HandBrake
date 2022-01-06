namespace HandBrakeWPF.Model.Options
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum RightToLeftMode
    {
        [DisplayName(typeof(Resources), "RightToLeft_Off")]
        Off = 0,

        [DisplayName(typeof(Resources), "RightToLeft_TextOnly")]
        TextOnly,

        [DisplayName(typeof(Resources), "RightToLeft_WholeInterface")]
        EntireInterface,
    }
}
