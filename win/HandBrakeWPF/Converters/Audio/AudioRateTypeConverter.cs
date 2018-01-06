// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioRateTypeConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio rate type converter.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Audio
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Windows.Data;

    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Utilities;

    using AudioEncoderRateType = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoderRateType;

    /// <summary>
    /// The audio rate type converter.
    /// </summary>
    public class AudioRateTypeConverter : IValueConverter
    {
        /// <summary>
        /// The convert.
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <param name="targetType">
        /// The target type.
        /// </param>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        /// <param name="culture">
        /// The culture.
        /// </param>
        /// <returns>
        /// The <see cref="object"/>.
        /// </returns>
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            var types = value as IEnumerable<AudioEncoderRateType>;
            if (types != null)
            {
                List<string> rateTypes = new List<string>();
                foreach (var item in types)
                {
                    rateTypes.Add(EnumHelper<AudioEncoderRateType>.GetDisplay(item));
                }

                return rateTypes;
            }

            if (targetType == typeof(AudioEncoderRateType) || value.GetType() == typeof(AudioEncoderRateType))
            {
                return EnumHelper<AudioEncoderRateType>.GetDisplay((AudioEncoderRateType)value);
            }

            return null;
        }

        /// <summary>
        /// The convert back.
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <param name="targetType">
        /// The target type.
        /// </param>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        /// <param name="culture">
        /// The culture.
        /// </param>
        /// <returns>
        /// The <see cref="object"/>.
        /// </returns>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value == null)
            {
                return null;
            }

            return EnumHelper<AudioEncoderRateType>.GetValue(value.ToString());
        }
    }
}
