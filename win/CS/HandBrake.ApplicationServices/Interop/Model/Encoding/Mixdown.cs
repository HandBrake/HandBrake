// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Mixdown.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Mixdown type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;
    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The Audio Mixdown Enumeration
    /// </summary>
    public enum Mixdown
    {
        [Display(Name = "Dolby Pro Logic II")]
        [ShortName("dpl2")]
        DolbyProLogicII = 0,

        [Display(Name = "None")]
        [ShortName("dpl2")]
        None,

        [Display(Name = "Automatic")]
        [ShortName("dpl2")]
        Auto,

        [Display(Name = "Mono")]
        [ShortName("mono")]
        Mono,

        [Display(Name = "Mono (Left Only)")]
        [ShortName("left_only")]
        LeftOnly,

        [Display(Name = "Mono (Right Only)")]
        [ShortName("right_only")]
        RightOnly,

        [Display(Name = "Stereo")]
        [ShortName("stereo")]
        Stereo,

        [Display(Name = "Dolby Surround")]
        [ShortName("dpl1")]
        DolbySurround,

        [Display(Name = "5.1 Channels")]
        [ShortName("5point1")]
        FivePoint1Channels,

        [Display(Name = "6.1 Channels")]
        [ShortName("6point1")]
        SixPoint1Channels,

        [Display(Name = "7.1 Channels")]
        [ShortName("7point1")]
        SevenPoint1Channels,

        [Display(Name = "7.1 (5F/2R/LFE)")]
        [ShortName("5_2_lfe")]
        Five_2_LFE,
    }
}
