// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CropMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Picture
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum CropMode
    {
        [DisplayName(typeof(Resources), "CropMode_Auto")]
        Automatic,

        [DisplayName(typeof(Resources), "CropMode_Loose")]
        Loose,

        [DisplayName(typeof(Resources), "CropMode_None")]
        None,

        [DisplayName(typeof(Resources), "CropMode_Custom")]
        Custom
    }
}