// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Detelecine.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Detelecine type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The detelecine.
    /// </summary>
    public enum Detelecine
    {
        [ShortName("off")]
        Off = 0,
        [ShortName("default")]
        Default = 2,
        [ShortName("custom")]
        Custom = 1
    }
}
