// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioMixdownListConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioMixdownListConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Audio
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Windows.Data;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Model.Encoding;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Utilities;

    /// <summary>
    /// The audio mixdown converter.
    /// Returns the list of available mixdowns for the given track and encoder.
    /// </summary>
    public class AudioMixdownListConverter : IValueConverter
    {
        /// <summary>
        /// Converts a value. 
        /// </summary>
        /// <returns>
        /// A converted value. If the method returns null, the valid null value is used.
        /// </returns>
        /// <param name="value">The value produced by the binding source.</param><param name="targetType">The type of the binding target property.</param><param name="parameter">The converter parameter to use.</param><param name="culture">The culture to use in the converter.</param>
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            AudioTrack track = value as AudioTrack;
            if (track != null && track.ScannedTrack != null)
            {
                HBAudioEncoder encoder =
                    HandBrakeEncoderHelpers.GetAudioEncoder(EnumHelper<AudioEncoder>.GetShortName(track.Encoder));
                           
                BindingList<HBMixdown> mixdowns = new BindingList<HBMixdown>();
                foreach (HBMixdown mixdown in HandBrakeEncoderHelpers.Mixdowns)
                {
                    if (HandBrakeEncoderHelpers.MixdownIsSupported(
                        mixdown,
                        encoder,
                        track.ScannedTrack.ChannelLayout))
                    {
                        mixdowns.Add(mixdown);
                    }
                }

                return mixdowns;
            }

            return value;
        }

        /// <summary>
        /// Converts a value. 
        /// </summary>
        /// <returns>
        /// A converted value. If the method returns null, the valid null value is used.
        /// </returns>
        /// <param name="value">The value that is produced by the binding target.</param><param name="targetType">The type to convert to.</param><param name="parameter">The converter parameter to use.</param><param name="culture">The culture to use in the converter.</param>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value;
        }
    }
}
