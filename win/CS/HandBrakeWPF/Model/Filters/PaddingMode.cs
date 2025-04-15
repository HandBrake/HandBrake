// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PaddingMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PaddingMode type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Filters
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum PaddingMode
    {
        [DisplayName(typeof(Resources), "PaddingMode_None")]
        [ShortName("none")]
        None = 0,

        [DisplayName(typeof(Resources), "PaddingMode_Height")]
        [ShortName("letterbox")]
        FillHeight,

        [DisplayName(typeof(Resources), "PaddingMode_Width")]
        [ShortName("pillarbox")]
        FillWidth,

        [DisplayName(typeof(Resources), "PaddingMode_Fill")]
        [ShortName("fill")]
        Fill,

        [DisplayName(typeof(Resources), "PaddingMode_Custom")]
        [ShortName("custom")]
        Custom,
    }
}
