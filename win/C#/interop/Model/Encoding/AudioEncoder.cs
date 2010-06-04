namespace HandBrake.Interop.Model.Encoding
{
    public enum AudioEncoder
    {
        [DisplayString("AAC (faac)")]
        Faac = 0,

        [DisplayString("MP3 (lame)")]
        Lame,

        [DisplayString("AC3 Passthrough")]
        Ac3Passthrough,

        [DisplayString("DTS Passthrough")]
        DtsPassthrough,

        [DisplayString("Vorbis (vorbis)")]
        Vorbis
    }
}
