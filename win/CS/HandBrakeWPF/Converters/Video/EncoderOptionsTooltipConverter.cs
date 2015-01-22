// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncoderOptionsTooltipConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The x 264 queue tooltip converter.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Video
{
    using System;
    using System.Globalization;
    using System.Windows.Data;

    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// The x 264 queue tooltip converter.
    /// </summary>
    public class EncoderOptionsTooltipConverter : IValueConverter
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
            EncodeTask task = value as EncodeTask;
            if (task != null && task.VideoEncoder == VideoEncoder.X264)
            {
                if (task.ShowAdvancedTab)
                {
                    return task.AdvancedEncoderOptions;
                }

                //return string.Format("Preset: {0}{5}Tune: {1}{5}Profile: {2}{5}Level: {3}{5}Extra Arguments: {4}{5}", 
                //    EnumHelper<x264Preset>.GetDisplay(task.X264Preset),
                //    EnumHelper<x264Tune>.GetDisplay(task.X264Tune),
                //    task.H264Profile,
                //    task.H264Level, 
                //    string.IsNullOrEmpty(task.ExtraAdvancedArguments) ? "None" : task.ExtraAdvancedArguments,
                //    Environment.NewLine);
            }

            return task != null ? task.AdvancedEncoderOptions.Trim() : string.Empty;
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
