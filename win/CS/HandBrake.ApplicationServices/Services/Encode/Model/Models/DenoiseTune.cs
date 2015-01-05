// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DenoiseTune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DenoiseTune type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode.Model.Models
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The denoise tune.
    /// </summary>
    public enum DenoiseTune
    {
        [Display(Name = "None")]
        None = 0,

        [Display(Name = "Film")]
        Film,

        [Display(Name = "Grain")]
        Grain,

        [Display(Name = "High Motion")]
        HighMotion,

        [Display(Name = "Animation")]
        Animation,
    }
}
