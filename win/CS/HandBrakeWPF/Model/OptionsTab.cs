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

    using HandBrakeWPF.Properties;

    /// <summary>
    /// A enum representing each tab on the options screen.
    /// </summary>
    public enum OptionsTab
    {
        [DisplayName(typeof(Resources), "Options_General")]
        General = 0,

        [DisplayName(typeof(Resources), "Options_OutputFiles")]
        OutputFiles,

        [DisplayName(typeof(Resources), "Options_WhenDone")]
        WhenDone,

        [DisplayName(typeof(Resources), "Options_Video")]
        Video,

        [DisplayName(typeof(Resources), "Options_Advanced")]
        Advanced,

        [DisplayName(typeof(Resources), "Options_Updates")]
        Updates,

        [DisplayName(typeof(Resources), "Options_About")]
        About,
    }
}
