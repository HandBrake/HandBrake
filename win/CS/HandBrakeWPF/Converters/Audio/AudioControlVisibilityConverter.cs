// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioControlVisibilityConverter.cs" company="HandBrake Project (http://handbrake.fr)">
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
                AudioEncoder fallbackEncoder = (AudioEncoder)values[2];

                if (!isVisibile)
                {
                    return Visibility.Collapsed;
                }

                // When the Fallback Encoder is "None" and we have a passthru encoder selected on the track, we don't have any encoder options to override so don't show them.
                if (isPassthru && fallbackEncoder == AudioEncoder.None)
                {
                    return Visibility.Collapsed;
                }
            }

            if (values.Length == 2)
            {
                bool isPassthru = (bool)values[0];
                AudioEncoder fallbackEncoder = (AudioEncoder)values[1];

                // When the Fallback Encoder is "None" and we have a passthru encoder selected on the track, we don't have any encoder options to override so don't show them.
                if (isPassthru && fallbackEncoder == AudioEncoder.None)
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