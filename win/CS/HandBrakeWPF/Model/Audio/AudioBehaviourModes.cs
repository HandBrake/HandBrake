// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioBehaviourModes.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio behaviours.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Audio
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    /// <summary>
    /// The audio behaviours.
    /// </summary>
    public enum AudioBehaviourModes
    {
        [DisplayName(typeof(Resources), "AudioBehaviourModes_None")]
        [ShortName("none")]
        None = 0,

        [DisplayName(typeof(Resources), "AudioBehaviourModes_FirstMatch")]
        [ShortName("first")]
        FirstMatch,

        [DisplayName(typeof(Resources), "AudioBehaviourModes_AllMatching")]
        [ShortName("all")]
        AllMatching,
    }
}
