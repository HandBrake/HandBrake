// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioQueueDisplayConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Audio Queue Display Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Audio
{
    using System;
    using System.Collections.ObjectModel;
    using System.Globalization;
    using System.Text;
    using System.Windows.Data;

    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Utilities;

    using AudioEncoder = HandBrakeWPF.Services.Encode.Model.Models.AudioEncoder;
    using AudioTrack = HandBrakeWPF.Services.Encode.Model.Models.AudioTrack;

    /// <summary>
    /// Audio Queue Display Converter
    /// </summary>
    public class AudioQueueDisplayConverter : IValueConverter
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
            ObservableCollection<AudioTrack> tracks = value as ObservableCollection<AudioTrack>;
            StringBuilder audioTracks = new StringBuilder();
            if (tracks != null)
            {
                foreach (AudioTrack track in tracks)
                {
                    string trackName = string.Format(
                        "{0} {1}",
                        track.ScannedTrack.TrackNumber,
                        track.ScannedTrack.Language);

                    audioTracks.Append(string.Format("{0} - {1} To {2}{3}", trackName, track.TrackName, EnumHelper<AudioEncoder>.GetDisplay(track.Encoder), Environment.NewLine)); 
                }
            }

            return audioTracks.ToString().Trim();
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
            throw new NotImplementedException();
        }
    }
}
