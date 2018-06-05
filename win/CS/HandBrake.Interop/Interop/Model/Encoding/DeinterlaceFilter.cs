// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DeinterlaceFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Deinterlace type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Model.Encoding
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The deinterlace.
    /// </summary>
    public enum DeinterlaceFilter
    {
        [ShortName("off")]
        Off = 0,

        [ShortName("Yadif")]
        Yadif = 1,

        [ShortName("Decomb")]
        Decomb = 2
    }
}
