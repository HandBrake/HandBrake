// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoEncoderConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Video Encoder Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Video
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;
    using SystemInfo = HandBrake.Interop.Utilities.SystemInfo;

    /// <summary>
    /// Video Encoder Converter
    /// </summary>
    public class VideoEncoderConverter : IMultiValueConverter
    {
        /// <summary>
        /// Gets a list of Video encoders OR returns the string name of an encoder depending on the input.
        /// </summary>
        /// <param name="values">
        /// The values.
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
        /// IEnumberable VideoEncoder or String encoder name.
        /// </returns>
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            if (values.Count() >= 2)
            {
                IUserSettingService userSettingService = values[2] as IUserSettingService;
                bool isQsvEnabled = false, isVceEnabled = false, isNvencEnabled = false;
                if (userSettingService != null)
                {
                    isQsvEnabled = userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncEncoding);
                    isVceEnabled = userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableVceEncoder);
                    isNvencEnabled = userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableNvencEncoder);
                }
                
                List<VideoEncoder> encoders = EnumHelper<VideoEncoder>.GetEnumList().ToList();
                EncodeTask task = values[1] as EncodeTask;

                if (HandBrakeEncoderHelpers.VideoEncoders.All(a => a.ShortName != EnumHelper<VideoEncoder>.GetShortName(VideoEncoder.X264_10)))
                {
                    encoders.Remove(VideoEncoder.X264_10);
                }

                if (HandBrakeEncoderHelpers.VideoEncoders.All(a => a.ShortName != EnumHelper<VideoEncoder>.GetShortName(VideoEncoder.X265_10)))
                {
                    encoders.Remove(VideoEncoder.X265_10);
                }

                if (HandBrakeEncoderHelpers.VideoEncoders.All(a => a.ShortName != EnumHelper<VideoEncoder>.GetShortName(VideoEncoder.X265_12)))
                {
                    encoders.Remove(VideoEncoder.X265_12);
                }

                if (task != null && task.OutputFormat == OutputFormat.Mp4)
                {
                    encoders.Remove(VideoEncoder.Theora);
                    encoders.Remove(VideoEncoder.VP8);
                    encoders.Remove(VideoEncoder.VP9);
                }
                else if (task != null && task.OutputFormat == OutputFormat.WebM)
                {
                    encoders.RemoveAll(ve => !(ve.Equals(VideoEncoder.VP9) || ve.Equals(VideoEncoder.VP8)));
                }

                if (!isQsvEnabled || !SystemInfo.IsQsvAvailableH264)
                {
                    encoders.Remove(VideoEncoder.QuickSync);
                }

                if (!isQsvEnabled || !SystemInfo.IsQsvAvailableH265)
                {
                    encoders.Remove(VideoEncoder.QuickSyncH265);
                    encoders.Remove(VideoEncoder.QuickSyncH26510b);
                }
                else if (!SystemInfo.IsQsvAvailableH26510bit)
                {
                    encoders.Remove(VideoEncoder.QuickSyncH26510b);
                }

                if (!isVceEnabled || !SystemInfo.IsVceH264Available)
                {
                    encoders.Remove(VideoEncoder.VceH264);
                }

                if (!isVceEnabled || !SystemInfo.IsVceH265Available)
                {
                    encoders.Remove(VideoEncoder.VceH265);
                }

                if (!isNvencEnabled || !SystemInfo.IsNVEncH264Available)
                {
                    encoders.Remove(VideoEncoder.NvencH264);
                }

                if (!isNvencEnabled || !SystemInfo.IsNVEncH265Available)
                {
                    encoders.Remove(VideoEncoder.NvencH265);
                }

                return EnumHelper<VideoEncoder>.GetEnumDisplayValuesSubset(encoders);
            }

            if (values[0].GetType() == typeof(VideoEncoder))
            {
                return EnumHelper<VideoEncoder>.GetDisplay((VideoEncoder)values[0]);
            }

            return null;
        }

        /// <summary>
        /// Convert from a string name, to enum value.
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <param name="targetTypes">
        /// The target types.
        /// </param>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        /// <param name="culture">
        /// The culture.
        /// </param>
        /// <returns>
        /// Returns the video encoder enum item.
        /// </returns>
        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            string name = value as string;
            if (!string.IsNullOrEmpty(name))
            {
                return new object[] { EnumHelper<VideoEncoder>.GetValue(name) };
            }

            return null;
        }
    }
}
