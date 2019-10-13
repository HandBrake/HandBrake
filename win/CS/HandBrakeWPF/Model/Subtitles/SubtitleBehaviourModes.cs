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

    using HandBrakeWPF.Properties;

    /// <summary>
    /// The subtitle behaviours modes.
    /// </summary>
    public enum SubtitleBehaviourModes
    {
        [DisplayName(typeof(Resources), "SubtitleBehaviourModes_None")]
        [ShortName("none")]
        None = 0,

        [DisplayName(typeof(Resources), "SubtitleBehaviourModes_FirstMatching")]
        [ShortName("first")]
        FirstMatch,

        [DisplayName(typeof(Resources), "SubtitleBehaviourModes_AllMatching")]
        [ShortName("all")]
        AllMatching,
    }
}
