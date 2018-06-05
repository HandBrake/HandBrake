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

    /// <summary>
    /// The audio behaviours.
    /// </summary>
    public enum AudioBehaviourModes
    {
        [DisplayName("No Audio")]
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
