// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PointToPointMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Point to Point Mode
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// Point to Point Mode
    /// </summary>
    public enum PointToPointMode
    {
        [Display(Name = "Chapters")]
        Chapters = 0,

        [Display(Name = "Seconds")]
        Seconds,

        [Display(Name = "Frames")]
        Frames,

        [Display(Name = "Preview")]
        Preview,
    }
}
