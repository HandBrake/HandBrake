// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PictureSettingsResLimitModes.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Picture
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum PictureSettingsResLimitModes
    {
        [DisplayName(typeof(Resources), "PictureSettingsResLimitModes_none")]
        None,

        [DisplayName(typeof(Resources), "PictureSettingsResLimitModes_8K")]
        [ResLimit(7680, 4320)]
        Size8K,

        [DisplayName(typeof(Resources), "PictureSettingsResLimitModes_4K")]
        [ResLimit(3840, 2160)]
        Size4K,

        [DisplayName(typeof(Resources), "PictureSettingsResLimitModes_1080p")]
        [ResLimit(1920, 1080)]
        Size1080p,

        [DisplayName(typeof(Resources), "PictureSettingsResLimitModes_720p")]
        [ResLimit(1280, 720)]
        Size720p,

        [DisplayName(typeof(Resources), "PictureSettingsResLimitModes_576p")]
        [ResLimit(720, 576)]
        Size576p,

        [DisplayName(typeof(Resources), "PictureSettingsResLimitModes_480p")]
        [ResLimit(720, 480)]
        Size480p,

        [DisplayName(typeof(Resources), "PictureSettingsResLimitModes_custom")]
        Custom,
    }
}