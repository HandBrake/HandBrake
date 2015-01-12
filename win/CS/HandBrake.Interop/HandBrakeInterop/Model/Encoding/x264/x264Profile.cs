// --------------------------------------------------------------------------------------------------------------------
// <copyright file="x264Profile.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The X264 Profile
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding.x264
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The X264 Profile
    /// </summary>
    public enum x264Profile
    {
        [Display(Name = "Auto")]
        Auto = 0,

        [Display(Name = "Baseline")]
        Baseline,

        [Display(Name = "Main")]
        Main,

        [Display(Name = "High")]
        High
    }
}
