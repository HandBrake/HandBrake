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
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The subtitle behaviours modes.
    /// </summary>
    public enum SubtitleBehaviourModes
    {
        [DisplayName("None")]
        [ShortName("none")]
        None = 0,

        [DisplayName("First Matching Selected Language")]
        [ShortName("first")]
        FirstMatch,

        [DisplayName("All Matching Selected Languages")]
        [ShortName("all")]
        AllMatching,
    }
}
