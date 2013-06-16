// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioBitrateConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Converter to provide the available audio bitrate options.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Audio
{
    using System;
    using System.Globalization;
    using System.Windows.Data;
    using System.Collections.Generic;
    using System.Linq;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// A Converter to provide the available audio bitrate options.
    /// </summary>
    public class AudioBitrateConverter : IValueConverter
    {
        /// <summary>
        /// The samplerates.
        /// </summary>
        readonly Dictionary<double, int> samplerates = new Dictionary<double, int>
               {
                   { 8, 8000 },
                   { 11.025, 11025 },
                   { 12, 12000 },
                   { 16, 16000 },
                   { 22.05, 22050 },
                   { 24, 24000 },
                   { 32, 32000 },
                   { 44.1, 44100 },
                   { 48, 48000 }
               };

        /// <summary>
        /// Converts source values to a value for the binding target. The data binding engine calls this method when it propagates the values from source bindings to the binding target.
        /// </summary>
        /// <returns>
        /// A converted value.If the method returns null, the valid null value is used.A return value of <see cref="T:System.Windows.DependencyProperty"/>.<see cref="F:System.Windows.DependencyProperty.UnsetValue"/> indicates that the converter did not produce a value, and that the binding will use the <see cref="P:System.Windows.Data.BindingBase.FallbackValue"/> if it is available, or else will use the default value.A return value of <see cref="T:System.Windows.Data.Binding"/>.<see cref="F:System.Windows.Data.Binding.DoNothing"/> indicates that the binding does not transfer the value or use the <see cref="P:System.Windows.Data.BindingBase.FallbackValue"/> or the default value.
        /// </returns>
        /// <param name="value">
        /// The value.
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
            // Base set of bitrates available.
            List<int> bitrates = new List<int> { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 640, 768, 960, 1152, 1344, 1536 };

            int max = 160;
            int low = 32;

            AudioTrack track = value as AudioTrack;
            if (track != null)
            {
                int samplerate = this.GetBestSampleRate(track);
                int srShift = this.GetSamplerateShift(samplerate);
                int lfeCount = this.GetLowFreqChannelCount(track.MixDown);
                int channels = this.GetDiscreteChannelCount(track.MixDown) - lfeCount;

                switch (track.Encoder)
                {
                    case AudioEncoder.ffaac:
                        low = ((channels + lfeCount) * 32);
                        max = ((channels + lfeCount) * ((192 + (64 * ((samplerate << srShift) >= 44100 ? 1 : 0))) >> srShift));
                        break;
                    case AudioEncoder.Lame:
                        low = 8 + (24 * (srShift < 1 ? 1 : 0) );
                        max = 64 + (96 * (srShift < 2 ? 1 : 0)) + (160 * (srShift < 1 ? 1 : 0));
                        break;
                    case AudioEncoder.Vorbis:
                        low = (channels + lfeCount) * (14 + (8 * (srShift < 2 ? 1 : 0)) + (6 * (srShift < 1 ? 1 : 0)));
                        max = (channels + lfeCount) * (32 + (54 * (srShift < 2 ? 1 : 0)) + (104 * (srShift < 1 ? 1 : 0)) + (50 * (samplerate >= 44100 ? 1 : 0)));
                        break;
                    case AudioEncoder.Ac3:
                        low = 224 * channels / 5;
                        max = 640;
                        break;
                    case AudioEncoder.Ac3Passthrough:
                    case AudioEncoder.DtsPassthrough:
                    case AudioEncoder.DtsHDPassthrough:
                    case AudioEncoder.AacPassthru:
                    case AudioEncoder.Mp3Passthru:
                    case AudioEncoder.Passthrough:
                    case AudioEncoder.ffflac:
                    case AudioEncoder.ffflac24:
                        max = 1536; // Since we don't care, just set it to the max.
                        break;
                    case AudioEncoder.fdkaac:
                        low = channels * samplerate * 2 / 3000;
                        max = channels * samplerate * 6 / 1000;
                        break;
                    case AudioEncoder.fdkheaac:
                        low = (channels * (12 + (4 * (samplerate >= 44100 ? 1 : 0))));
                        max = (channels - (channels > 2 ? 1 : 0)) * (48 + (16 * (samplerate >= 22050 ? 1 : 0)));
                        break;
                    default:
                        max = 768;
                        break;
                }

                // Bring the bitrate down in-line with the max.
                if (track.Bitrate < low)
                {
                    track.Bitrate = low;
                }

                if (track.Bitrate > max)
                {
                    track.Bitrate = max;
                }
            }

            return bitrates.Where(bitrate => bitrate <= max && bitrate >= low);
        }

        /// <summary>
        /// The get channel count.
        /// </summary>
        /// <param name="mixdown">
        /// The mixdown.
        /// </param>
        /// <returns>
        /// The System.Int32.
        /// </returns>
        private int GetDiscreteChannelCount(Mixdown mixdown)
        {
            switch (mixdown)
            {
                case Mixdown.Five_2_LFE:
                case Mixdown.SevenPoint1Channels:
                    return 8;
                case Mixdown.SixPoint1Channels:
                    return 7;
                case Mixdown.FivePoint1Channels:
                    return 6;
                case Mixdown.Mono:
                case Mixdown.LeftOnly:
                case Mixdown.RightOnly:
                    return 1;
                case Mixdown.None:
                    return 0;
                default:
                    return 2;
            }
        }

        /// <summary>
        /// The get low freq channel count.
        /// </summary>
        /// <param name="mixdown">
        /// The mixdown.
        /// </param>
        /// <returns>
        /// The System.Int32.
        /// </returns>
        private int GetLowFreqChannelCount(Mixdown mixdown)
        {
            switch (mixdown)
            {
                case Mixdown.FivePoint1Channels:
                case Mixdown.SixPoint1Channels:
                case Mixdown.SevenPoint1Channels:
                case Mixdown.Five_2_LFE:
                    return 1;
                default:
                    return 0;
            }
        }

        /// <summary>
        /// The get samplerate shift.
        /// </summary>
        /// <param name="samplerate">
        /// The samplerate.
        /// </param>
        /// <returns>
        /// The System.Int32.
        /// </returns>
        private int GetSamplerateShift(int samplerate)
        {
            /* sr_shift: 0 -> 48000, 44100, 32000 Hz
             *           1 -> 24000, 22050, 16000 Hz
             *           2 -> 12000, 11025,  8000 Hz
             *
             * also, since samplerates are sanitized downwards:
             *
             * (samplerate < 32000) implies (samplerate <= 24000)
             */
            return ((samplerate < 16000) ? 2 : (samplerate < 32000) ? 1 : 0);
        }

        /// <summary>
        /// The get best sample rate.
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        /// <returns>
        /// The System.Double.
        /// </returns>
        private int GetBestSampleRate(AudioTrack track)
        {
            int samplerate = 48000; // Default to 48

            // Try get the users selected sample rate
            if (!track.SampleRate.Equals(0.0d))
            {
                samplerate = this.samplerates[track.SampleRate];
            }
            else if (track.ScannedTrack != null && track.ScannedTrack.SampleRate != 0) // If it's auto, try get the source
            {
                samplerate = track.ScannedTrack.SampleRate;
            }

            // THen Sanitize to make sure it's valid
            int bestSamplerate;
            if ((samplerate < 32000) && (track.Encoder == AudioEncoder.Ac3))
            {
                // AC-3 < 32 kHz suffers from poor hardware compatibility
                bestSamplerate = 32000;
            } 
            else if ((samplerate < 16000) && (track.Encoder == AudioEncoder.fdkheaac))
            {
                bestSamplerate = 16000;
            }
            else
            {
                bestSamplerate = samplerate;
                foreach (KeyValuePair<double, int> item in this.samplerates)
                {
                    // valid samplerate
                    if (bestSamplerate.Equals(item.Value))
                        break;

                    // samplerate is higher than the next valid samplerate,
                    // or lower than the lowest valid samplerate
                    if (bestSamplerate > item.Value && bestSamplerate < this.samplerates.First().Value)
                    {
                        bestSamplerate = item.Value;
                        break;
                    }
                }
            }

            return bestSamplerate;
        }

        /// <summary>
        /// The convert back.
        /// </summary>
        /// <param name="value">
        /// The value.
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
        /// The System.Object.
        /// </returns>
        /// <exception cref="NotImplementedException">
        /// We don't use this.
        /// </exception>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
