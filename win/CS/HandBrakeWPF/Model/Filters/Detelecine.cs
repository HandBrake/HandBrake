// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Detelecine.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Detelecine type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Filters
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    /// <summary>
    /// The detelecine.
    /// </summary>
    public enum Detelecine
    {
        [DisplayName(typeof(Resources), "Detelecine_Off")]		
        [ShortName("off")]
        Off = 0,
		
        [DisplayName(typeof(Resources), "Detelecine_Default")]
        [ShortName("default")]
        Default = 2,
		
        [DisplayName(typeof(Resources), "Detelecine_Custom")]
        [ShortName("custom")]
        Custom = 1
    }
}