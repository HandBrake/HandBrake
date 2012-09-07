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
            List<int> bitrates = new List<int> { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 640, 768 };

            int max = 160;
            AudioTrack track = value as AudioTrack;
            if (track != null)
            {
                switch (track.Encoder)
                {
                    case AudioEncoder.Faac:
                    case AudioEncoder.ffaac:
                        max = track.MixDown == Mixdown.FivePoint1Channels ? 768 : 320;
                        break;
                    case AudioEncoder.Lame:
                        max = 320;
                        break;
                    case AudioEncoder.Vorbis:
                        max = 384;
                        break;
                    case AudioEncoder.Ac3:
                        max = 640;
                        break;
                    case AudioEncoder.Ac3Passthrough:
                    case AudioEncoder.DtsPassthrough:
                    case AudioEncoder.DtsHDPassthrough:
                    case AudioEncoder.AacPassthru:
                    case AudioEncoder.Mp3Passthru:
                    case AudioEncoder.Passthrough:
                    case AudioEncoder.ffflac:
                        max = 768; // Since we don't care, just set it to the max.
                        break;
                    default:
                        max = 768;
                        break;
                }

                // Bring the bitrate down in-line with the max.
                if (track.Bitrate > max)
                {
                    track.Bitrate = max;
                }
            }

            return bitrates.Where(bitrate => bitrate <= max);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
