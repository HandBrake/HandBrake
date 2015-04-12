// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoScaler.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The different scaling modes available in HandBrake
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    ///  The different scaling modes available in HandBrake
    /// </summary>
    public enum VideoScaler
    {
        [Display(Name = "Lanczos (default)")]
        Lanczos = 0,

        [Display(Name = "Bicubic (OpenCL)")]
        BicubicCl,
    }
}
