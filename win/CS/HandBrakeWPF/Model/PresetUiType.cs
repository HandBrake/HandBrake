// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AppThemeMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum PresetUiType
    {
        [DisplayName(typeof(Resources), "PresetUiType_Menu")]
        Menu,

        [DisplayName(typeof(Resources), "PresetUiType_Overlay")]
        Overlay
    }
}