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

    /// <summary>
    /// The subtitle behaviours modes.
    /// </summary>
    public enum SubtitleBurnInBehaviourModes
    {
        [DisplayName("None")]
        [ShortName("none")]
        None = 0,

        [DisplayName("Foreign Audio Track")]
        [ShortName("foreign")]
        ForeignAudio,

        [DisplayName("First Track")]
        [ShortName("first")]
        FirstTrack,

        [DisplayName("Foreign Audio Preferred, else First")]
        [ShortName("foreign_first")]
        ForeignAudioPreferred,
    }
}
