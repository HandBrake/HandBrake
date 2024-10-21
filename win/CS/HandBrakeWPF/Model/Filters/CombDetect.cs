// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CombDetect.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the CombDetect type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Filters
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    /// <summary>
    /// The CombDetect Type.
    /// </summary>
    public enum CombDetect
    {
        [DisplayName(typeof(Resources), "CombDetect_Off")]
		[ShortName("off")]
        Off,

        [DisplayName(typeof(Resources), "CombDetect_Custom")]
        [ShortName("custom")]
        Custom,

        [DisplayName(typeof(Resources), "CombDetect_Default")]
        [ShortName("default")]
        Default,

        [ShortName("permissive")]
        LessSensitive,

        [ShortName("fast")]
        Fast
    }
}