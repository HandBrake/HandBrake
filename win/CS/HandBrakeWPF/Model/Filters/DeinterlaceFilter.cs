// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DeinterlaceFilter.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Deinterlace type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Filters
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    /// <summary>
    /// The deinterlace.
    /// </summary>
    public enum DeinterlaceFilter
    {
        [DisplayName(typeof(Resources), "DeinterlaceFilter_Off")]
        [ShortName("off")]
        Off = 0,

        [DisplayName(typeof(Resources), "DeinterlaceFilter_Yadif")]
        [ShortName("Yadif")]
        Yadif = 1,

        [DisplayName(typeof(Resources), "DeinterlaceFilter_Decomb")]
        [ShortName("Decomb")]
        Decomb = 2,

        [DisplayName(typeof(Resources), "DeinterlaceFilter_Bwdif")]
        [ShortName("Bwdif")]
        Bwdif = 3,
    }
}