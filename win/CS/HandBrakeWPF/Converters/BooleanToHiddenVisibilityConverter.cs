// --------------------------------------------------------------------------------------------------------------------
// <copyright file="BooleanToHiddenVisibilityConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the BooleanToHiddenVisibilityConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters
{
    using System;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Data;

    /// <summary>
    /// Boolean to Visibility Converter (Hidden, not Collapsed)
    /// </summary>
    public sealed class BooleanToHiddenVisibilityConverter : IValueConverter
    {
        /// <summary>
        /// Convert a boolean to visibility property.
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
            // Parameter is a boolean which inverts the output.
            var param = System.Convert.ToBoolean(parameter, CultureInfo.InvariantCulture);

            if (value == null)
            {
                return Visibility.Hidden;
            }

            if (value is bool)
            {
                if (param)
                {
                    return (bool)value ? Visibility.Hidden : Visibility.Visible;
                }
                else
                {
                    return (bool)value ? Visibility.Visible : Visibility.Hidden;
                }
            }

            return value;
        }

        /// <summary>
        /// Convert Back for the IValueConverter Interface. Not used!
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
            throw new NotImplementedException();
        }
    }
}
