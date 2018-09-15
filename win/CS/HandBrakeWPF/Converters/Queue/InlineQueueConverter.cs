// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InlineQueueConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Converter to handle the UI re-arrangement for in-line queue.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Queue
{
    using System;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Data;

    public class InlineQueueConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            // Parameter = True is Flipped logic.
            if (parameter is string && "true".Equals(parameter))
            {
                if (value is bool && (bool)value)
                {
                    return new GridLength(1, GridUnitType.Star);
                }

                return GridLength.Auto;
            }

            // Parameter = False
            if (value is bool && (bool)value)
            {
                return GridLength.Auto;
            }

            return new GridLength(1, GridUnitType.Star);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return new GridLength(1, GridUnitType.Star);
        }
    }
}
