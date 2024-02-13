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
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Scan.Model;

    using OutputFormat = Services.Encode.Model.Models.OutputFormat;

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
        /// IEnumerable AudioEncoder or String encoder name.
        /// </returns>
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            if (values.Length >= 2)
            {
                List<HBAudioEncoder> encoders = HandBrakeEncoderHelpers.AudioEncoders.ToList();
                
                OutputFormat outputFormat = OutputFormat.Mp4;
                if (values[1].GetType() == typeof(OutputFormat))
                {
                    outputFormat = (OutputFormat)values[1];
                }
                else
                {
                    EncodeTask task = values[1] as EncodeTask;
                    if (task != null)
                    {
                        outputFormat = task.OutputFormat;
                    }
                }

                foreach (HBAudioEncoder encoder in HandBrakeEncoderHelpers.AudioEncoders)
                {
                    if (outputFormat == OutputFormat.Mp4 && !encoder.SupportsMP4)
                    {
                        encoders.Remove(encoder);
                    }
                    else if (outputFormat == OutputFormat.Mkv && !encoder.SupportsMkv)
                    {
                        encoders.Remove(encoder);
                    }
                    else if (outputFormat == OutputFormat.WebM && !encoder.SupportsWebM)
                    {
                        encoders.Remove(encoder);
                    }
                }
                
                // Special Mode: Hide the Passthru options and show the "None" option
                if (parameter != null && parameter.ToString() == "True")
                {
                    foreach (HBAudioEncoder encoder in HandBrakeEncoderHelpers.AudioEncoders)
                    {
                        if (encoder.IsPassthrough)
                        {
                            encoders.Remove(encoder);
                        }
                    }
                    
                    encoders.Add(HBAudioEncoder.None);
                }

                if (values.Length == 3)
                {
                    encoders.Remove(HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.Passthru)); // Auto passthru doesn't make sense on the main window. instead only show supported passthrus. 

                    Audio sourceTrack = values[2] as Audio;
                    foreach (HBAudioEncoder encoder in HandBrakeEncoderHelpers.AudioEncoders)
                    {
                        if (encoder.IsPassthrough)
                        {
                            RemoveIfNotSupported(encoder, sourceTrack, encoders);
                        }
                    }
                }

                return encoders;
            }

            if (values.Any() && values.First() != DependencyProperty.UnsetValue)
            {
                return (HBAudioEncoder)values[0];
            }

            return HandBrakeEncoderHelpers.GetAudioEncoder(HBAudioEncoder.AvAac);     
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            return new object[] { value as HBAudioEncoder };
        }

        private void RemoveIfNotSupported(HBAudioEncoder encoder, Audio sourceTrack, List<HBAudioEncoder> encoders)
        {
            if (sourceTrack == null)
            {
                return;
            }

            if ((sourceTrack.Codec & encoder.Id) == 0)
            {
                encoders.Remove(encoder);
            }
        }
    }
}
