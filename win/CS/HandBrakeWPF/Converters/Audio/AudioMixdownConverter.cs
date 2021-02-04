// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioMixdownConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioMixdownConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Audio
{
    using System;
    using System.Globalization;
    using System.Windows.Data;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    /// <summary>
    /// The audio mixdown converter.
    /// Handles conversion between HBMixdown and it's shortname.
    /// </summary>
    public class AudioMixdownConverter : IValueConverter
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
            string mixdown = value as string;
            if (!string.IsNullOrEmpty(mixdown))
            {
                return HandBrakeEncoderHelpers.GetMixdown(mixdown);
            }

            return HandBrakeEncoderHelpers.GetMixdown("dpl2"); // Default
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
            HBMixdown mixdown = value as HBMixdown;
            if (mixdown != null)
            {
                return mixdown.ShortName;
            }

            return "none";
        }
    }
}
