// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoOptionsTooltipConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video options queue tooltip converter.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Video
{
    using System;
    using System.Globalization;
    using System.Windows.Data;

    using HandBrake.Interop.Interop;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using VideoEncodeRateType = HandBrakeWPF.Model.Video.VideoEncodeRateType;

    /// <summary>
    /// The x 264 queue tooltip converter.
    /// </summary>
    public class VideoOptionsTooltipConverter : IValueConverter
    {
        /// <summary>
        /// Converts a value. 
        /// </summary>
        /// <returns>
        /// A converted value. If the method returns null, the valid null value is used.
        /// </returns>
        /// <param name="value">
        /// The value produced by the binding source.
        /// </param>
        /// <param name="targetType">
        /// The type of the binding target property.
        /// </param>
        /// <param name="parameter">
        /// The converter parameter to use.
        /// </param>
        /// <param name="culture">
        /// The culture to use in the converter.
        /// </param>
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            EncodeTask task = value as EncodeTask;
            if (task != null)
            {
                string rfqp = HandBrakeEncoderHelpers.GetVideoQualityRateControlName(task.VideoEncoder.ShortName);

                string quality = task.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality ? string.Format("{0} {1}", task.Quality, rfqp) : string.Format("{0} {1}", task.VideoBitrate, " kbps");
                string multiPass = null;

                if (task.VideoEncodeRateType == VideoEncodeRateType.AverageBitrate)
                {
                    multiPass = task.MultiPass ? task.TurboAnalysisPass ? " (Multi-Pass with Turbo)" : " (Multi-Pass)" : string.Empty;
                }

                return string.Format("{0} - {1}{2}", task.VideoEncoder.DisplayName, quality, multiPass); 
            }

            return "Unknown";
        }

        /// <summary>
        /// Converts a value. 
        /// </summary>
        /// <returns>
        /// A converted value. If the method returns null, the valid null value is used.
        /// </returns>
        /// <param name="value">
        /// The value that is produced by the binding target.
        /// </param>
        /// <param name="targetType">
        /// The type to convert to.
        /// </param>
        /// <param name="parameter">
        /// The converter parameter to use.
        /// </param>
        /// <param name="culture">
        /// The culture to use in the converter.
        /// </param>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
