// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitlesQueueDisplayConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Subtitle Queue Display Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Subtitles
{
    using System;
    using System.Collections.ObjectModel;
    using System.Globalization;
    using System.Text;
    using System.Windows.Data;

    using SubtitleTrack = HandBrakeWPF.Services.Encode.Model.Models.SubtitleTrack;

    /// <summary>
    /// Subtitle Queue Display Converter
    /// </summary>
    public class SubtitlesQueueDisplayConverter : IValueConverter
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
            ObservableCollection<SubtitleTrack> tracks = value as ObservableCollection<SubtitleTrack>;
            StringBuilder sutitleTracks = new StringBuilder();
            if (tracks != null)
            {
                foreach (SubtitleTrack track in tracks)
                {
                    string text = track.SourceTrack != null
                                      ? track.SourceTrack.ToString()
                                      : (track.SrtFileName + ".srt");

                    sutitleTracks.AppendLine(text);
                }
            }

            return string.IsNullOrEmpty(sutitleTracks.ToString()) ? "None" : sutitleTracks.ToString().Trim();
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
