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
    using System.Runtime.InteropServices;
    using System.Windows.Data;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;

    using EncodeTask = Services.Encode.Model.EncodeTask;
    using OutputFormat = Services.Encode.Model.Models.OutputFormat;
    using VideoEncoder = Model.Video.VideoEncoder;

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
        /// IEnumerable VideoEncoder or String encoder name.
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
                
                List<VideoEncoder> allEncoders = values[0] as List<VideoEncoder>;
                EncodeTask task = values[1] as EncodeTask;

                if (task == null || allEncoders == null)
                {
                    return null;
                }

                List<VideoEncoder> returnEncoders = new List<VideoEncoder>(allEncoders);

                foreach (var encoder in allEncoders)
                {
                    HBVideoEncoder foundEncoder = HandBrakeEncoderHelpers.GetVideoEncoder(EnumHelper<VideoEncoder>.GetShortName(encoder));
                    if (foundEncoder == null)
                    {
                        returnEncoders.Remove(encoder);
                        continue;
                    }

                    if (task.OutputFormat == OutputFormat.Mp4 && !foundEncoder.SupportsMP4)
                    {
                        returnEncoders.Remove(encoder);
                    }

                    if (task.OutputFormat == OutputFormat.Mkv && !foundEncoder.SupportsMKV)
                    {
                        returnEncoders.Remove(encoder);
                    }

                    if (task.OutputFormat == OutputFormat.WebM && !foundEncoder.SupportsWebM)
                    {
                        returnEncoders.Remove(encoder);
                    }

                    if (!isQsvEnabled && VideoEncoderHelpers.IsQuickSync(encoder))
                    {
                        returnEncoders.Remove(encoder);
                    }

                    // Older generation parts seem to be having increasing problems and are no longer recieving driver feature updates. 
                    // Support Skylake (6th gen) and newer.
                    if (HandBrakeHardwareEncoderHelper.QsvHardwareGeneration < 6 && VideoEncoderHelpers.IsQuickSync(encoder))
                    {
                        returnEncoders.Remove(encoder);
                    }

                    if (!isVceEnabled && VideoEncoderHelpers.IsVCN(encoder))
                    {
                        returnEncoders.Remove(encoder);
                    }

                    if (!isNvencEnabled && VideoEncoderHelpers.IsNVEnc(encoder))
                    {
                        returnEncoders.Remove(encoder);
                    }
                }

                return EnumHelper<VideoEncoder>.GetEnumDisplayValuesSubset(returnEncoders);
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
