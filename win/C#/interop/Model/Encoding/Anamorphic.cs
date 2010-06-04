// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Anamorphic.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Anamorphic type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// Anamorphic Modes
    /// </summary>
    public enum Anamorphic
    {
        [DisplayString("None")]
        None = 0,
        [DisplayString("Strict")]
        Strict,
        [DisplayString("Loose")]
        Loose,
        [DisplayString("Custom")]
        Custom
    }
}
