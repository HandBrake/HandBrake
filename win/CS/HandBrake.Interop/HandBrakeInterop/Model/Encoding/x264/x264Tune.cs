// --------------------------------------------------------------------------------------------------------------------
// <copyright file="x264Tune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the x264Tune type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding.x264
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The X264 Tune MOdel
    /// </summary>
    public enum x264Tune
    {
        [Display(Name = "Film")]
        Film = 0,

        [Display(Name = "Animation")]
        Animation,

        [Display(Name = "Grain")]
        Grain,

        [Display(Name = "Still Image")]
        Stillimage,

        [Display(Name = "PSNR")]
        Psnr,

        [Display(Name = "SSIM")]
        Ssim,

        [Display(Name = "Fast Decode")]
        Fastdecode,

        [Display(Name = "Zero Latency")]
        Zerolatency,
    }
}
