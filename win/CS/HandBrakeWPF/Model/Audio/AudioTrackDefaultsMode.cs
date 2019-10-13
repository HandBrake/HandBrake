// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioTrackDefaultsMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio track defaults mode.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Audio
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    /// <summary>
    /// The audio behaviours.
    /// </summary>
    public enum AudioTrackDefaultsMode
    {
        [DisplayName(typeof(Resources), "AudioBehaviourModes_FirstTrack")]
        FirstTrack = 0,

        [DisplayName(typeof(Resources), "AudioBehaviourModes_AllTracks")]
        AllTracks,
    }
}
