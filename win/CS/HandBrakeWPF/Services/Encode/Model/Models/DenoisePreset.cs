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
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The denoise preset.
    /// </summary>
    public enum DenoisePreset
    {
        [DisplayName("Weak")]
        [ShortName("weak")]
        Weak = 0,

        [DisplayName("Medium")]
        [ShortName("medium")]
        Medium,

        [DisplayName("Strong")]
        [ShortName("strong")]
        Strong,

        [DisplayName("Custom")]
        [ShortName("custom")]
        Custom,

        [DisplayName("Ultralight")] // NLMeans only
        [ShortName("ultralight")]
        Ultralight,

        [DisplayName("Light")] // NLMeans only
        [ShortName("light")]
        Light,
    }
}
