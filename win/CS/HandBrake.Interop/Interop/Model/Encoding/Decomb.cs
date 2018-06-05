// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Decomb.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Decomb type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Model.Encoding
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The decomb.
    /// </summary>
    public enum Decomb
    {
        [ShortName("default")]
        Default,

        [ShortName("bob")]
        Bob,

        [ShortName("custom")]
        Custom,

        [ShortName("eedi2")]
        EEDI2,

        [ShortName("eedi2bob")]
        EEDI2Bob
    }
}
