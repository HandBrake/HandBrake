// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsTabConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Options Tab Converter. Controls which tab is dispalyed.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Options
{
    using System;
    using System.Globalization;
    using System.Windows;
    using System.Windows.Data;

    using HandBrakeWPF.Model;

    /// <summary>
    /// The Options Tab Converter. Controls which tab is dispalyed.
    /// </summary>
    public class OptionsTabConverter : IValueConverter
    {
        /// <summary>
        /// Converts a value. 
        /// </summary>
        /// <returns>
        /// A converted value. If the method returns null, the valid null value is used.
        /// </returns>
        /// <param name="value">The value produced by the binding source.</param><param name="targetType">The type of the binding target property.</param><param name="parameter">The converter parameter to use.</param><param name="culture">The culture to use in the converter.</param>
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value != null && parameter != null)
            {
                switch ((OptionsTab)value)
                {
                    case OptionsTab.General:
                        if ((OptionsTab)parameter == OptionsTab.General) return Visibility.Visible;
                        break;
                    case OptionsTab.OutputFiles:
                        if ((OptionsTab)parameter == OptionsTab.OutputFiles) return Visibility.Visible;
                        break;
                    case OptionsTab.WhenDone:
                        if ((OptionsTab)parameter == OptionsTab.WhenDone) return Visibility.Visible;
                        break;
                    case OptionsTab.Advanced:
                        if ((OptionsTab)parameter == OptionsTab.Advanced) return Visibility.Visible;
                        break;
                    case OptionsTab.Updates:
                        if ((OptionsTab)parameter == OptionsTab.Updates) return Visibility.Visible;
                        break;
                    case OptionsTab.About:
                        if ((OptionsTab)parameter == OptionsTab.About) return Visibility.Visible;
                        break;
                    case OptionsTab.Video:
                        if ((OptionsTab)parameter == OptionsTab.Video) return Visibility.Visible;
                        break;
                }
            }

            return Visibility.Collapsed;
        }

        /// <summary>
        /// Converts a value. 
        /// </summary>
        /// <returns>
        /// A converted value. If the method returns null, the valid null value is used.
        /// </returns>
        /// <param name="value">The value that is produced by the binding target.</param><param name="targetType">The type to convert to.</param><param name="parameter">The converter parameter to use.</param><param name="culture">The culture to use in the converter.</param>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
