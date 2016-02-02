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
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Windows.Data;

    /// <summary>
    /// The log level converter.
    /// </summary>
    public class LogLevelConverter : IValueConverter
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
            if (value.GetType() == typeof(BindingList<int>))
            {
                return new List<string> { "Minimised", "Standard", "Extended" };
            }
            else if (value is int)
            {
                switch ((int)value)
                {
                    case 0:
                        return "Minimised";
                    case 2:
                        return "Extended";
                    default:
                        return "Standard";
                }
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
            switch ((string)value)
            {
                case "Minimised":
                    return 0;
                case "Extended":
                    return 2;
                default:
                case "Standard":
                    return 1;
            }
        }
    }
}
