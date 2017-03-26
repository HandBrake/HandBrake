// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueStatusToVisibilityConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the QueueStatusToVisibilityConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters
{
    using System;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Data;
    using HandBrakeWPF.Services.Queue.Model;

    /// <summary>
    /// Boolean to Visibility Converter
    /// </summary>
    public sealed class QueueStatusToVisibilityConverter : IValueConverter
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
            if (value != null)
            {
                QueueItemStatus status = (QueueItemStatus)value;
                switch (status)
                {
                    case QueueItemStatus.Waiting:
                    case QueueItemStatus.InProgress:
                        return Visibility.Collapsed;
                    default:
                        return Visibility.Visible;
                }
            }

            return Visibility.Collapsed;
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
