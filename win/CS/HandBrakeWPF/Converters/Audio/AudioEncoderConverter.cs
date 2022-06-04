﻿// --------------------------------------------------------------------------------------------------------------------
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

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Encoders;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.Utilities;

    using AudioEncoder = Services.Encode.Model.Models.AudioEncoder;
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
            if (values.Count() >= 2)
            {
                List<AudioEncoder> encoders = EnumHelper<AudioEncoder>.GetEnumList().ToList();

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

                encoders.Remove(AudioEncoder.None); // Assume we never want to show this.

                if (!HandBrakeEncoderHelpers.AudioEncoders.Any(a => a.ShortName.Contains("fdk")))
                {
                    encoders.Remove(AudioEncoder.fdkaac);
                    encoders.Remove(AudioEncoder.fdkheaac);
                }

                if (outputFormat == OutputFormat.Mp4)
                {
                    encoders.Remove(AudioEncoder.Vorbis);
                    encoders.Remove(AudioEncoder.ffflac);
                    encoders.Remove(AudioEncoder.ffflac24);
                    encoders.Remove(AudioEncoder.FlacPassthru);

                    encoders.Remove(AudioEncoder.TrueHDPassthrough);
                }
                else if (outputFormat == OutputFormat.WebM)
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
                    encoders.Remove(AudioEncoder.Mp2Passthru);
                    encoders.Remove(AudioEncoder.Passthrough);
                    encoders.Remove(AudioEncoder.TrueHDPassthrough);
                    encoders.Remove(AudioEncoder.FlacPassthru);
                    encoders.Remove(AudioEncoder.OpusPassthru);

                    encoders.Add(AudioEncoder.None);
                }

                if (values.Length == 3)
                {
                    encoders.Remove(AudioEncoder.Passthrough); // Auto passthru doesn't make sense on the main window. instead only show supported passthrus. 

                    Audio sourceTrack = values[2] as Audio;
                    RemoveIfNotSupported(AudioEncoder.DtsHDPassthrough, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.DtsPassthrough, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.EAc3Passthrough, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.AacPassthru, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.Ac3Passthrough, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.Mp3Passthru, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.TrueHDPassthrough, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.FlacPassthru, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.Mp2Passthru, sourceTrack, encoders);
                    RemoveIfNotSupported(AudioEncoder.OpusPassthru, sourceTrack, encoders);
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

        private void RemoveIfNotSupported(AudioEncoder encoder, Audio sourceTrack, List<AudioEncoder> encoders)
        {
            if (sourceTrack == null)
            {
                return;
            }

            HBAudioEncoder encoderInfo = HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(encoder));
            if ((sourceTrack.Codec & encoderInfo.Id) == 0)
            {
                encoders.Remove(encoder);
            }
        }
    }
}
