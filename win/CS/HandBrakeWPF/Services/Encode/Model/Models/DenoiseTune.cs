// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DenoiseTune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DenoiseTune type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System.ComponentModel.DataAnnotations;

    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The denoise tune.
    /// </summary>
    public enum DenoiseTune
    {
        [Display(Name = "None")]
        [ShortName("none")]
        None = 0,

        [Display(Name = "Film")]
        [ShortName("film")]
        Film,

        [Display(Name = "Grain")]
        [ShortName("grain")]
        Grain,

        [Display(Name = "High Motion")]
        [ShortName("highmotion")]
        HighMotion,

        [Display(Name = "Animation")]
        [ShortName("animation")]
        Animation,
    }
}
