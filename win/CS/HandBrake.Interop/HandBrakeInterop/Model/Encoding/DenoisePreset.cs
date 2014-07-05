// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DenoisePreset.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DenoisePreset type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The denoise preset.
    /// </summary>
    public enum DenoisePreset
    {
        [Display(Name = "Weak")]
        Weak = 0,

        [Display(Name = "Medium")]
        Medium,

        [Display(Name = "Strong")]
        Strong,

        [Display(Name = "Custom")]
        Custom,

        [Display(Name = "Ultralight")] // NLMeans only
        Ultralight,

        [Display(Name = "Light")] // NLMeans only
        Light,
    }
}
