// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FullPathToFileNameConverter.cs" company="HandBrake Project (http://handbrake.fr)">
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
    using System.IO;
    using System.Windows.Data;

    /// <summary>
    /// Converts a Full Path to Filename only.
    /// </summary>
    public sealed class FullPathToFileNameConverter : IValueConverter
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
            string path = value as string;
            if (!string.IsNullOrEmpty(path) && !path.EndsWith("\\"))
            {
                return Path.GetFileName(path);
            }

            return "Unknown";
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
