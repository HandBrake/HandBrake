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
    public enum Deinterlace
    {
        [ShortName("off")]
        Off = 0,

        [ShortName("fast")]
        Fast = 2,

        [ShortName("slow")]
        Slow = 3,

        [ShortName("slower")]
        Slower = 4,

        [ShortName("bob")]
        Bob = 5,

        [ShortName("custom")]
        Custom = 1
    }
}
