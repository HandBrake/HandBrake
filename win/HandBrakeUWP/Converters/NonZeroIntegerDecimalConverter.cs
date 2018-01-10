// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NonZeroIntegerDecimalConverter.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Converters
{
    using System;
    using Windows.UI.Xaml.Data;

    public class NonZeroIntegerDecimalConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is int integer)
            {
                if (integer == 0)
                {
                    return null;
                }

                return System.Convert.ToDouble(integer);
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            if (value == null)
            {
                return 0;
            }

            if (value is double val)
            {
                return System.Convert.ToInt32(val);
            }

            return value;
        }
    }
}