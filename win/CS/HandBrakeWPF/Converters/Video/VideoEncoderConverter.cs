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

    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.Services.Interfaces;

    using EncodeTask = Services.Encode.Model.EncodeTask;
    using OutputFormat = Services.Encode.Model.Models.OutputFormat;

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

                IEnumerable<HBVideoEncoder> allEncoders = values[0] as IEnumerable<HBVideoEncoder>;
                EncodeTask task = values[1] as EncodeTask;

                if (task == null || allEncoders == null)
                {
                    return null;
                }

                List<HBVideoEncoder> returnEncoders = new List<HBVideoEncoder>(allEncoders);

                foreach (var encoder in allEncoders)
                {
                    if (encoder == null)
                    {
                        returnEncoders.Remove(encoder);
                        continue;
                    }

                    if (task.OutputFormat == OutputFormat.Mp4 && !encoder.SupportsMP4)
                    {
                        returnEncoders.Remove(encoder);
                    }

                    if (task.OutputFormat == OutputFormat.Mkv && !encoder.SupportsMKV)
                    {
                        returnEncoders.Remove(encoder);
                    }

                    if (task.OutputFormat == OutputFormat.WebM && !encoder.SupportsWebM)
                    {
                        returnEncoders.Remove(encoder);
                    }
                }

                return returnEncoders;
            }

            if (values[0].GetType() == typeof(HBVideoEncoder))
            {
                return (HBVideoEncoder)values[0];
            }

            return null;
        }


        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            return new object[] { value as HBVideoEncoder };
        }
    }
}
