// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsTabNameConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Converter to get the Display Name of each options tab.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Options
{
    using System;
    using System.Globalization;
    using System.Windows.Data;

    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Utilities;

    /// <summary>
    /// A Converter to get the Display Name of each options tab.
    /// </summary>
    public class OptionsTabNameConverter : IValueConverter
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
            return EnumHelper<OptionsTab>.GetDisplay((OptionsTab)value);
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
            return EnumHelper<OptionsTab>.GetValue(value.ToString());
        }
    }
}