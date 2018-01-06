// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FileSizeConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the LogLevelConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Options
{
    using System;
    using System.Globalization;
    using System.Windows.Data;

    public class FileSizeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value != null)
            {
                return (long)value / 1000 / 1000 / 1000;
            }

            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            long size;
            if (value != null && long.TryParse(value.ToString(), out size))
            {
                return size * 1000 * 1000 * 1000;
            }

            return null;
        }
    }
}
