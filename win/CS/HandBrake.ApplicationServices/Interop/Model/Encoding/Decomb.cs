// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Decomb.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Decomb type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    /// <summary>
    /// The decomb.
    /// </summary>
    public enum Decomb
    {
        Off = 0,
        Default = 2,
        Fast = 3,
        Bob = 4,
        Custom = 1
    }
}
