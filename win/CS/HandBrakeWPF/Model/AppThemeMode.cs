// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AppThemeMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum AppThemeMode
    {
        [DisplayName(typeof(Resources), "DarkTheme_light")]
        Light,

        [DisplayName(typeof(Resources), "DarkTheme_dark")]
        Dark,

        [DisplayName(typeof(Resources), "DarkTheme_system")]
        System,

        [DisplayName(typeof(Resources), "DarkTheme_Modern")]
        Modern,

        [DisplayName(typeof(Resources), "DarkTheme_None")]
        None,
    }
}