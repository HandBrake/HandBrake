// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Deinterlace.cs" company="HandBrake Project (http://handbrake.fr)">
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
    public enum Deinterlace
    {
        [ShortName("custom")]
        Custom,

        [ShortName("default")]
        Default,

        [ShortName("skip-spatial")]
        SkipSpatialCheck,

        [ShortName("bob")]
        Bob
    }
}
