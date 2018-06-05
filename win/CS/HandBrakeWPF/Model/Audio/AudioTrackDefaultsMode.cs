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

    /// <summary>
    /// The audio behaviours.
    /// </summary>
    public enum AudioTrackDefaultsMode
    {
        [DisplayName("Use First Track as template")]
        FirstTrack = 0,

        [DisplayName("Use All Tracks as templates")]
        AllTracks,
    }
}
