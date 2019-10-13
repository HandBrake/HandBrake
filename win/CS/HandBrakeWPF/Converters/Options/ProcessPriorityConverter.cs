// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ProcessPriorityConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Process Priority Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Options
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Utilities;

    public class ProcessPriorityConverter : ResourceConverterBase<ProcessPriority>, IValueConverter
    {
    }
}
