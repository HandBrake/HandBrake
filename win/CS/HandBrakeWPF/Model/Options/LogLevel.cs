// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogLevel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the LogLevel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Options
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum LogLevel
    {
        [DisplayName(typeof(Resources), "LogLevel_Minimised")]
        Minimised = 0,

        [DisplayName(typeof(Resources), "LogLevel_Standard")]
        Standard,

        [DisplayName(typeof(Resources), "LogLevel_Extended")]
        Extended = 2
    }
}