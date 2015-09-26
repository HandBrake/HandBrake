// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DenoisePreset.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DenoisePreset type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System.ComponentModel.DataAnnotations;

    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The denoise preset.
    /// </summary>
    public enum DenoisePreset
    {
        [Display(Name = "Weak")]
        [ShortName("weak")]
        Weak = 0,

        [Display(Name = "Medium")]
        [ShortName("medium")]
        Medium,

        [Display(Name = "Strong")]
        [ShortName("strong")]
        Strong,

        [Display(Name = "Custom")]
        [ShortName("custom")]
        Custom,

        [Display(Name = "Ultralight")] // NLMeans only
        [ShortName("ultralight")]
        Ultralight,

        [Display(Name = "Light")] // NLMeans only
        [ShortName("light")]
        Light,
    }
}
