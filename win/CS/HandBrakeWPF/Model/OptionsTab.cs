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
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// A enum representing each tab on the options screen.
    /// </summary>
    public enum OptionsTab
    {
        [DisplayName("General")]
        General = 0,

        [DisplayName("Output Files")]
        OutputFiles,

        [DisplayName("Video")]
        Video,

        [DisplayName("Advanced")]
        Advanced,

        [DisplayName("Updates")]
        Updates,

        [DisplayName("About HandBrake")]
        About,
    }
}
