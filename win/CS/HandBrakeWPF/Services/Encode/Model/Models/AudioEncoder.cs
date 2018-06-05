// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio encoder enumeration
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The audio encoder.
    /// </summary>
    public enum AudioEncoder
    {
        [DisplayName("None")]
        [ShortName("none")]
        None,

        [DisplayName("AAC (avcodec)")]
        [ShortName("av_aac")]
        ffaac,

        [DisplayName("AAC (FDK)")]
        [ShortName("fdk_aac")]
        fdkaac,

        [DisplayName("HE-AAC (FDK)")]
        [ShortName("fdk_haac")]
        fdkheaac,

        [DisplayName("MP3")]
        [ShortName("mp3")]
        Lame,

        [DisplayName("AC3")]
        [ShortName("ac3")]
        Ac3,

        [DisplayName("Auto Passthru")]
        [ShortName("copy")]
        Passthrough,

        [DisplayName("AC3 Passthru")]
        [ShortName("copy:ac3")]
        Ac3Passthrough,

        [DisplayName("E-AC3 Passthru")]
        [ShortName("copy:eac3")]
        EAc3Passthrough,

        [DisplayName("DTS Passthru")]
        [ShortName("copy:dts")]
        DtsPassthrough,

        [DisplayName("DTS-HD Passthru")]
        [ShortName("copy:dtshd")]
        DtsHDPassthrough,

        [DisplayName("TrueHD Passthru")]
        [ShortName("copy:truehd")]
        TrueHDPassthrough,

        [DisplayName("AAC Passthru")]
        [ShortName("copy:aac")]
        AacPassthru,

        [DisplayName("MP3 Passthru")]
        [ShortName("copy:mp3")]
        Mp3Passthru,

        [DisplayName("Vorbis")]
        [ShortName("vorbis")]
        Vorbis,

        [DisplayName("FLAC 16-bit")]
        [ShortName("flac16")]
        ffflac,

        [DisplayName("FLAC 24-bit")]
        [ShortName("flac24")]
        ffflac24,

        [DisplayName("FLAC Passthru")]
        [ShortName("copy:flac")]
        FlacPassthru,

        [DisplayName("Opus (libopus)")]
        [ShortName("opus")]
        Opus,
    }
}
