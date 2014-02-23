// --------------------------------------------------------------------------------------------------------------------
// <copyright file="x265Profile.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The X265 Profile
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding.x265
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The X265 Profile
    /// </summary>
    public enum x265Profile
    {
        [Display(Name = "Auto")]
        None = 0,

        [Display(Name = "Main")]
        Main,

        [Display(Name = "Main10")]
        Main10,

        [Display(Name = "Mainstillpicture")]
        Mainstillpicture,
    }
}