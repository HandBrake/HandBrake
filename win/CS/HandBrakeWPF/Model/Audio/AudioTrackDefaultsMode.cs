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
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The audio behaviours.
    /// </summary>
    public enum AudioTrackDefaultsMode
    {
        [Display(Name = "Default")]
        None = 0,

        [Display(Name = "Use First Track as template")]
        FirstTrack,

        [Display(Name = "Use All Tracks as templates")]
        AllTracks,
    }
}
