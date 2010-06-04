// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Deinterlace.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Deinterlace type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// Delinterlace Mode
    /// </summary>
    public enum Deinterlace
    {
        Off = 0,
        Fast,
        Slow,
        Slower,
        Custom
    }
}
