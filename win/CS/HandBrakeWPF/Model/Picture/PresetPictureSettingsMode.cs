// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetPictureSettingsMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Picture Settings Mode when adding presets
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Picture
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// Picture Settings Mode when adding presets
    /// </summary>
    public enum PresetPictureSettingsMode
    {
        [DisplayName("None")]
        None = 0,
        [DisplayName("Custom")]
        Custom = 1,
        [DisplayName("Always use Source Resolution")]
        SourceMaximum = 2,
    }
}