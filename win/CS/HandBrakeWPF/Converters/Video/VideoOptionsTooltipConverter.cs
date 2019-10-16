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

    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.Utilities;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

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
                string rfqp = task.VideoEncoder == VideoEncoder.X264 || task.VideoEncoder == VideoEncoder.X264_10 || task.VideoEncoder == VideoEncoder.X265 
                    || task.VideoEncoder == VideoEncoder.X265_10 || task.VideoEncoder == VideoEncoder.X265_12 ? "RF" : "QP";

                if (task.VideoEncoder == VideoEncoder.NvencH264 || task.VideoEncoder == VideoEncoder.NvencH265)
                {
                    rfqp = "CQ";
                }

                string quality = task.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality ? string.Format("{0} {1}", task.Quality, rfqp) : string.Format("{0} {1}", task.VideoBitrate, " kbps");
                string twoPass = null;

                if (task.VideoEncodeRateType == VideoEncodeRateType.AverageBitrate)
                {
                    twoPass = task.TwoPass ? task.TurboFirstPass ? " (2-Pass with Turbo)" : " (2-Pass)" : string.Empty;
                }

                return string.Format("{0} - {1}{2}", EnumHelper<VideoEncoder>.GetDisplay(task.VideoEncoder), quality, twoPass); 
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
