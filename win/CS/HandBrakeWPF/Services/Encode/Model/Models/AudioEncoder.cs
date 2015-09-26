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
    using System.ComponentModel.DataAnnotations;

    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The audio encoder.
    /// </summary>
    public enum AudioEncoder
    {
        [Display(Name = "AAC (avcodec)")]
        [ShortName("av_aac")]
        ffaac,

        [Display(Name = "AAC (FDK)")]
        [ShortName("fdk_aac")]
        fdkaac,

        [Display(Name = "HE-AAC (FDK)")]
        [ShortName("fdk_haac")]
        fdkheaac,

        [Display(Name = "MP3")]
        [ShortName("mp3")]
        Lame,

        [Display(Name = "AC3")]
        [ShortName("ac3")]
        Ac3,

        [Display(Name = "Auto Passthru")]
        [ShortName("copy")]
        Passthrough,

        [Display(Name = "AC3 Passthru")]
        [ShortName("copy:ac3")]
        Ac3Passthrough,

        [Display(Name = "E-AC3 Passthru")]
        [ShortName("copy:eac3")]
        EAc3Passthrough,

        [Display(Name = "DTS Passthru")]
        [ShortName("copy:dts")]
        DtsPassthrough,

        [Display(Name = "DTS-HD Passthru")]
        [ShortName("copy:dtshd")]
        DtsHDPassthrough,

        [Display(Name = "TrueHD Passthru")]
        [ShortName("copy:truehd")]
        TrueHDPassthrough,

        [Display(Name = "AAC Passthru")]
        [ShortName("copy:aac")]
        AacPassthru,

        [Display(Name = "MP3 Passthru")]
        [ShortName("copy:mp3")]
        Mp3Passthru,

        [Display(Name = "Vorbis")]
        [ShortName("vorbis")]
        Vorbis,

        [Display(Name = "FLAC 16-bit")]
        [ShortName("flac16")]
        ffflac,

        [Display(Name = "FLAC 24-bit")]
        [ShortName("flac24")]
        ffflac24,

        [Display(Name = "FLAC Passthru")]
        [ShortName("copy:flac")]
        FlacPassthru,
    }
}
