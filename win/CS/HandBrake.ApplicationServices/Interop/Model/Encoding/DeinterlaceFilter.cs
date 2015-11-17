// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Deinterlace.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Deinterlace type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The deinterlace.
    /// </summary>
    public enum DeinterlaceFilter
    {
        [ShortName("off")]
        Off = 0,

        [ShortName("Deinterlace")]
        Deinterlace = 1,

        [ShortName("Decomb")]
        Decomb = 2
    }
}
