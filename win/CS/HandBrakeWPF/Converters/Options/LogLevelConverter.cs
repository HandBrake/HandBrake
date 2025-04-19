// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogLevelConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the LogLevelConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Options
{
    using System.Windows.Data;

    using HandBrakeWPF.Model.Options;
	
    public class LogLevelConverter : ResourceConverterBase<LogLevel>, IValueConverter
    {
    }
}