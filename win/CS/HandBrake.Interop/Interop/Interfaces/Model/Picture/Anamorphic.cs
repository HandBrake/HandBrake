// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Anamorphic.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Anamorphic type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Picture
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The anamorphic.
    /// </summary>
    public enum Anamorphic
    {
        [DisplayName("None")]
        [ShortName("none")]
        None = 0,
        [DisplayName("Automatic")]
        [ShortName("auto")]
        Automatic = 4,
        [DisplayName("Custom")]
        [ShortName("custom")]
        Custom = 3
    }
}