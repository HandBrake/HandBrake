// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleBurnInBehaviourModes.cs" company="HandBrake Project (http://handbrake.fr)">
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
    public enum SubtitleBurnInBehaviourModes
    {
        [DisplayName(typeof(Resources), "SubtitleBurnInBehaviourModes_None")]
        [ShortName("none")]
        None = 0,

        [DisplayName(typeof(Resources), "SubtitleBurnInBehaviourModes_ForeignAudioTrack")]
        [ShortName("foreign")]
        ForeignAudio,

        [DisplayName(typeof(Resources), "SubtitleBurnInBehaviourModes_FirstTrack")]
        [ShortName("first")]
        FirstTrack,

        [DisplayName(typeof(Resources), "SubtitleBurnInBehaviourModes_ForeignAudioPreferredElseFirst")]
        [ShortName("foreign_first")]
        ForeignAudioPreferred,
    }
}
