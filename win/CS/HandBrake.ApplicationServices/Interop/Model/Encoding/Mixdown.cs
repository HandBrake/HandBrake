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

    /// <summary>
    /// The Audio Mixdown Enumeration
    /// </summary>
    public enum Mixdown
    {
        [Display(Name = "Dolby Pro Logic II")]
        DolbyProLogicII = 0,

        [Display(Name = "None")]
        None,

        [Display(Name = "Automatic")]
        Auto,

        [Display(Name = "Mono")]
        Mono,

        [Display(Name = "Mono (Left Only)")]
        LeftOnly,

        [Display(Name = "Mono (Right Only)")]
        RightOnly,

        [Display(Name = "Stereo")]
        Stereo,

        [Display(Name = "Dolby Surround")]
        DolbySurround,

        [Display(Name = "5.1 Channels")]
        FivePoint1Channels,

        [Display(Name = "6.1 Channels")]
        SixPoint1Channels,

        [Display(Name = "7.1 Channels")]
        SevenPoint1Channels,

        [Display(Name = "7.1 (5F/2R/LFE)")]
        Five_2_LFE,
    }
}
