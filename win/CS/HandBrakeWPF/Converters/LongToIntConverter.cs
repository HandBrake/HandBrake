// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LongToIntConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the FullPathToFileNameConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters
{
    using System;
    using System.Globalization;
    using System.Windows.Data;

    public class LongToIntConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value != null && value is long && (long)value <= int.MaxValue)
            {
                long result = (long)value;
                return (int)result;
            }

            return (int)0;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value;
        }
    }
}
