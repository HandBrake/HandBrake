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
    using System.ComponentModel.DataAnnotations;

    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The subtitle behaviours modes.
    /// </summary>
    public enum SubtitleBurnInBehaviourModes
    {
        [Display(Name = "None")]
        [ShortName("none")]
        None = 0,

        [Display(Name = "Foreign Audio Track")]
        [ShortName("foreign")]
        ForeignAudio,

        [Display(Name = "First Track")]
        [ShortName("first")]
        FirstTrack,

        [Display(Name = "Foreign Audio Preferred, else First")]
        [ShortName("foreign_first")]
        ForeignAudioPreferred,
    }
}
