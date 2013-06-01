// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoderConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Audio Encoder Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Audio
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// Audio Encoder Converter
    /// </summary>
    public class AudioEncoderConverter : IMultiValueConverter
    {
        /// <summary>
        /// Gets a list of audio encoders OR returns the string name of an encoder depending on the input.
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
        /// IEnumberable AudioEncoder or String encoder name.
        /// </returns>
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            // TODO -> Be smarter and only show the available Passthru options.
            if (values.Count() == 2)
            {
                List<AudioEncoder> encoders = EnumHelper<AudioEncoder>.GetEnumList().ToList();
                EncodeTask task = values[1] as EncodeTask;

                if (task != null && task.OutputFormat != OutputFormat.Mkv)
                {
                    encoders.Remove(AudioEncoder.Vorbis);
                    encoders.Remove(AudioEncoder.ffflac);
                    encoders.Remove(AudioEncoder.ffflac24);
                }

                if (parameter != null && parameter.ToString() == "True")
                {
                    encoders.Remove(AudioEncoder.DtsHDPassthrough);
                    encoders.Remove(AudioEncoder.DtsPassthrough);
                    encoders.Remove(AudioEncoder.AacPassthru);
                    encoders.Remove(AudioEncoder.Ac3Passthrough);
                    encoders.Remove(AudioEncoder.Mp3Passthru);
                    encoders.Remove(AudioEncoder.Passthrough);
                }

                return EnumHelper<AudioEncoder>.GetEnumDisplayValuesSubset(encoders);
            }

            return EnumHelper<AudioEncoder>.GetDisplay((AudioEncoder)values[0]);
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
        /// Returns the audio encoder enum item.
        /// </returns>
        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            string name = value as string;
            if (!string.IsNullOrEmpty(name))
            {
                return new object[] { EnumHelper<AudioEncoder>.GetValue(name)};
            }

            return null;
        }
    }
}
