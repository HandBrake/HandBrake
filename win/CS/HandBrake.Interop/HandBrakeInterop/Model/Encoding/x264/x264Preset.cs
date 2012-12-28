// --------------------------------------------------------------------------------------------------------------------
// <copyright file="x264Preset.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the x264Preset type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding.x264
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The X264 Preset
    /// </summary>
    public enum x264Preset
    {
        [Display(Name = "Ultrafast")]
        Ultrafast,

        [Display(Name = "Super Fast")]
        Superfast,

        [Display(Name = "Very Fast")]
        VeryFast,

        [Display(Name = "Faster")]
        Faster,

        [Display(Name = "Fast")]
        Fast,

        [Display(Name = "Medium")]
        Medium,

        [Display(Name = "Slow")]
        Slow,

        [Display(Name = "Slower")]
        Slower,

        [Display(Name = "Very Slow")]
        VerySlow,

        [Display(Name = "Placebo")]
        Placebo,
    }
}
