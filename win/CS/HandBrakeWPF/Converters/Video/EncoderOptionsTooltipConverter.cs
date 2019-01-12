// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncoderOptionsTooltipConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The x 264 queue tooltip converter.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Video
{
    using System;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrake.Interop.Interop.Model.Encoding;

    using EncodeTask = Services.Encode.Model.EncodeTask;
    using VideoLevel = Services.Encode.Model.Models.Video.VideoLevel;
    using VideoPreset = Services.Encode.Model.Models.Video.VideoPreset;
    using VideoProfile = Services.Encode.Model.Models.Video.VideoProfile;
    using VideoTune = Services.Encode.Model.Models.Video.VideoTune;

    public class EncoderOptionsTooltipConverter : IValueConverter
    {
        /// <summary>
        /// Converts a value. 
        /// </summary>
        /// <returns>
        /// A converted value. If the method returns null, the valid null value is used.
        /// </returns>
        /// <param name="value">
        /// The value produced by the binding source.
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
            EncodeTask task = value as EncodeTask;
            if (task != null && (task.VideoEncoder == VideoEncoder.X264 || task.VideoEncoder == VideoEncoder.X264_10 || task.VideoEncoder == VideoEncoder.X265 || task.VideoEncoder == VideoEncoder.X265_10 || task.VideoEncoder == VideoEncoder.X265_12))
            {
                VideoTune tune = task.VideoTunes.FirstOrDefault();

                return string.Format("Preset: {0}{5}Tune: {1}{5}Profile: {2}{5}Level: {3}{5}Extra Arguments: {4}{5}",
                    task.VideoPreset != null ? task.VideoPreset.ShortName : VideoPreset.None.DisplayName,
                    tune != null ? tune.ShortName : VideoTune.None.DisplayName,
                    task.VideoProfile != null ? task.VideoProfile.ShortName : VideoProfile.Auto.DisplayName,
                    task.VideoLevel != null ? task.VideoLevel.ShortName : VideoLevel.Auto.DisplayName, 
                    string.IsNullOrEmpty(task.ExtraAdvancedArguments) ? "None" : task.ExtraAdvancedArguments, 
                    Environment.NewLine);
            }

            return string.Empty;
        }

        /// <summary>
        /// Converts a value. 
        /// </summary>
        /// <returns>
        /// A converted value. If the method returns null, the valid null value is used.
        /// </returns>
        /// <param name="value">
        /// The value that is produced by the binding target.
        /// </param>
        /// <param name="targetType">
        /// The type to convert to.
        /// </param>
        /// <param name="parameter">
        /// The converter parameter to use.
        /// </param>
        /// <param name="culture">
        /// The culture to use in the converter.
        /// </param>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
