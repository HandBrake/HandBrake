namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    /// <summary>
    /// Anamorphic Mode
    /// </summary>
    public enum Anamorphic
    {
        [Description("None")]
        None = 0,
        [Description("Strict")]
        Strict,
        [Description("Loose")]
        Loose,
        [Description("Custom")]
        Custom
    }
}
