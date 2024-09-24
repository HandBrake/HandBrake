// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Anamorphic.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Anamorphic type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Picture
{
    using HandBrake.Interop.Attributes;
    using HandBrakeWPF.Properties;

    /// <summary>
    /// The anamorphic.
    /// </summary>
    public enum AnamorphicMode
    {
        [DisplayName(typeof(Resources), "AnamorphicMode_None")]
        [ShortName("none")]
        None = 0,
        [DisplayName(typeof(Resources), "AnamorphicMode_Automatic")]
        [ShortName("auto")]
        Automatic = 4,
        [DisplayName(typeof(Resources), "AnamorphicMode_Custom")]
        [ShortName("custom")]
        Custom = 3
    }
}