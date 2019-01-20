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
    using System.Windows;
    using System.Windows.Data;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Utilities;

    using AudioEncoder = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder;
    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;

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

                encoders.Remove(AudioEncoder.None); // Assume we never want to show this.

                if (!HandBrakeEncoderHelpers.AudioEncoders.Any(a => a.ShortName.Contains("fdk")))
                {
                    encoders.Remove(AudioEncoder.fdkaac);
                    encoders.Remove(AudioEncoder.fdkheaac);
                }

                if (task != null && task.OutputFormat == OutputFormat.Mp4)
                {
                    encoders.Remove(AudioEncoder.Vorbis);
                    encoders.Remove(AudioEncoder.ffflac);
                    encoders.Remove(AudioEncoder.ffflac24);
                    encoders.Remove(AudioEncoder.FlacPassthru);
                    encoders.Remove(AudioEncoder.Opus);

                    encoders.Remove(AudioEncoder.TrueHDPassthrough);
                }
                else if (task != null && task.OutputFormat == OutputFormat.WebM)
                {
                    encoders.RemoveAll(ae => !(ae.Equals(AudioEncoder.Vorbis) || ae.Equals(AudioEncoder.Opus)));
                }

                // Hide the Passthru options and show the "None" option
                if (parameter != null && parameter.ToString() == "True")
                {
                    encoders.Remove(AudioEncoder.DtsHDPassthrough);
                    encoders.Remove(AudioEncoder.DtsPassthrough);
                    encoders.Remove(AudioEncoder.EAc3Passthrough);
                    encoders.Remove(AudioEncoder.AacPassthru);
                    encoders.Remove(AudioEncoder.Ac3Passthrough);
                    encoders.Remove(AudioEncoder.Mp3Passthru);
                    encoders.Remove(AudioEncoder.Passthrough);
                    encoders.Remove(AudioEncoder.TrueHDPassthrough);
                    encoders.Remove(AudioEncoder.FlacPassthru);

                    encoders.Add(AudioEncoder.None);
                }

                return EnumHelper<AudioEncoder>.GetEnumDisplayValuesSubset(encoders);
            }

            if (values.Any() && values.First() != DependencyProperty.UnsetValue)
            {
                return EnumHelper<AudioEncoder>.GetDisplay((AudioEncoder)values[0]);
            }

            return EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.ffaac);           
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
                return new object[] { EnumHelper<AudioEncoder>.GetValue(name) };
            }

            return null;
        }
    }
}
