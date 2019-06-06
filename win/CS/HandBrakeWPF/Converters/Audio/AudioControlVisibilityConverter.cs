// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioControlVisibilityConverter.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Audio
{
    using System;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Data;

    using HandBrakeWPF.Services.Encode.Model.Models;

    public class AudioControlVisibilityConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            if (values.Length == 3)
            {
                bool isVisibile = (bool)values[0];
                bool isPassthru = (bool)values[1];
                AudioEncoder fallbackEncoder = AudioEncoder.ffaac;

                if (values[2] != null && values[2].GetType() == typeof(AudioEncoder))
                {
                    fallbackEncoder = (AudioEncoder)values[2];
                }

                if (!isVisibile)
                {
                    return Visibility.Collapsed;
                }

                if (fallbackEncoder == AudioEncoder.None && isPassthru)
                {
                    return Visibility.Collapsed;
                }
            }

            return Visibility.Visible;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}