// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsTab.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A enum representing each tab on the options screen.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// A enum representing each tab on the options screen.
    /// </summary>
    public enum OptionsTab
    {
        [Display(Name = "General")]
        General = 0,

        [Display(Name = "Output Files")]
        OutputFiles,

        [Display(Name = "Audio and Subtitles")]
        AudioAndSubtitles,

        [Display(Name = "Advanced")]
        Advanced,

        [Display(Name = "Updates")]
        Updates,

        [Display(Name = "About HandBrake")]
        About,
    }
}
