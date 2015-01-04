// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleBehaviourModes.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The subtitle behaviours modes.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Subtitles
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The subtitle behaviours modes.
    /// </summary>
    public enum SubtitleBehaviourModes
    {
        [Display(Name = "None")]
        None = 0,

        [Display(Name = "First Matching Selected Language")]
        FirstMatch,

        [Display(Name = "All Matching Selected Languages")]
        AllMatching,
    }
}
