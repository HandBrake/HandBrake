// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EnumComboConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EnumComboConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters
{
    using System.Collections.Generic;
    using System.Globalization;
    using System.Windows.Data;
    using System;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.Interop.Model.Encoding;
    using HandBrake.Interop.Model.Encoding.x264;

    /// <summary>
    /// Enum Combo Converter
    /// </summary>
    public sealed class EnumComboConverter : IValueConverter
    {
        /// <summary>
        /// Convert an Enum to it's display value (attribute)
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <param name="targetType">
        /// The target type.
        /// </param>
        /// <param name="parameter">
        /// The parameter. (A boolean which inverts the output)
        /// </param>
        /// <param name="culture">
        /// The culture.
        /// </param>
        /// <returns>
        /// Visibility property
        /// </returns>
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            // Lists
            if (value is IEnumerable<x264Preset>)
            {
                return EnumHelper<x264Preset>.GetEnumDisplayValues(typeof(x264Preset));
            }
            if (value is IEnumerable<x264Profile>)
            {
                return EnumHelper<x264Profile>.GetEnumDisplayValues(typeof(x264Profile));
            }
            if (value is IEnumerable<x264Tune>)
            {
                return EnumHelper<x264Tune>.GetEnumDisplayValues(typeof(x264Tune));
            }
            if (value is IEnumerable<VideoEncoder>)
            {
                return EnumHelper<VideoEncoder>.GetEnumDisplayValues(typeof(VideoEncoder));
            }
            if (value is IEnumerable<Mixdown>)
            {
                return EnumHelper<Mixdown>.GetEnumDisplayValues(typeof(Mixdown));
            }

            if (value is IEnumerable<AudioEncoder>)
            {
                return EnumHelper<AudioEncoder>.GetEnumDisplayValues(typeof(AudioEncoder));
            }



            // Single Items
            if (targetType == typeof(x264Preset) || value.GetType() == typeof(x264Preset))
            {
                return EnumHelper<x264Preset>.GetDisplay((x264Preset)value);
            }
            if (targetType == typeof(x264Profile) || value.GetType() == typeof(x264Profile))
            {
                return EnumHelper<x264Profile>.GetDisplay((x264Profile)value);
            }
            if (targetType == typeof(x264Tune) || value.GetType() == typeof(x264Tune))
            {
                return EnumHelper<x264Tune>.GetDisplay((x264Tune)value);
            }
            if (targetType == typeof(VideoEncoder) || value.GetType() == typeof(VideoEncoder))
            {
                return EnumHelper<VideoEncoder>.GetDisplay((VideoEncoder)value);
            }
            if (targetType == typeof(Mixdown) || value.GetType() == typeof(Mixdown))
            {
                return EnumHelper<Mixdown>.GetDisplay((Mixdown)value);
            }
            if (targetType == typeof(AudioEncoder) || value.GetType() == typeof(AudioEncoder))
            {
                return EnumHelper<AudioEncoder>.GetDisplay((AudioEncoder)value);
            }

            return null;
        }

        /// <summary>
        /// Convert Back for the IValueConverter Interface. 
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
        /// Nothing
        /// </returns>
        /// <exception cref="NotImplementedException">
        /// This method is not used!
        /// </exception>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (targetType == typeof(x264Preset) || value.GetType() == typeof(x264Preset))
            {
                return EnumHelper<x264Preset>.GetValue(value.ToString());
            }
            if (targetType == typeof(x264Profile) || value.GetType() == typeof(x264Profile))
            {
                return EnumHelper<x264Profile>.GetValue(value.ToString());
            }
            if (targetType == typeof(x264Tune) || value.GetType() == typeof(x264Tune))
            {
                return EnumHelper<x264Tune>.GetValue(value.ToString());
            }
            if (targetType == typeof(VideoEncoder) || value.GetType() == typeof(VideoEncoder))
            {
                return EnumHelper<VideoEncoder>.GetValue(value.ToString());
            }
            if (targetType == typeof(Mixdown) || value.GetType() == typeof(Mixdown))
            {
                return EnumHelper<Mixdown>.GetValue(value.ToString());
            }
            if (targetType == typeof(AudioEncoder) || value.GetType() == typeof(AudioEncoder))
            {
                return EnumHelper<AudioEncoder>.GetValue(value.ToString());
            }

            return null;
        }
    }
}
