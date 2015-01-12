// --------------------------------------------------------------------------------------------------------------------
// <copyright file="x265Tune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the x265Tune type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding.x265
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The X265 Tune MOdel
    /// </summary>
    public enum x265Tune
    {
        [Display(Name = "None")]
        None = 0,

        [Display(Name = "PSNR")]
        psnr,

        [Display(Name = "SSIM")]
        ssim,

        [Display(Name = "Zero Latency")]
        zerolatency,

        [Display(Name = "Fast Decode")]
        fastdecode,

    }
}