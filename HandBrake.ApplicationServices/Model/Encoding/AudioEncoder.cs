/*  AudioEncoder.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    using HandBrake.ApplicationServices.Converters;

    [TypeConverter(typeof(EnumToDescConveter))]
    public enum AudioEncoder
    {
        [Description("AAC (faac)")]
        Faac = 0,

        [Description("AAC (ffmpeg)")]
        ffaac,

        [Description("MP3 (lame)")]
        Lame,

        [Description("AC3 (ffmpeg)")]
        Ac3,

        [Description("Passthru")]
        Passthrough,

        [Description("AC3 Passthru")]
        Ac3Passthrough,

        [Description("DTS Passthru")]
        DtsPassthrough,

        [Description("Vorbis (vorbis)")]
        Vorbis
    }
}
