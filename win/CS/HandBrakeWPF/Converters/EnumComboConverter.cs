// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EnumComboConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EnumComboConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Windows.Data;
    using HandBrake.Interop.Interop.Model.Encoding;
    using HandBrake.Interop.Model;

    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Utilities;
    using OutputFormat = HandBrakeWPF.Services.Encode.Model.Models.OutputFormat;
    using PresetPictureSettingsMode = HandBrakeWPF.Model.Picture.PresetPictureSettingsMode;

    /// <summary>
    /// Enum Combo Converter
    /// </summary>
    public sealed class EnumComboConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value == null)
            {
                return null;
            }

            // Lists
            if (value is IEnumerable<VideoEncoder>)
            {
                return EnumHelper<VideoEncoder>.GetEnumDisplayValues(typeof(VideoEncoder));
            }

            if (value is IEnumerable<PresetPictureSettingsMode>)
            {
                return EnumHelper<PresetPictureSettingsMode>.GetEnumDisplayValues(typeof(PresetPictureSettingsMode));
            }

            if (value is IEnumerable<Detelecine>)
            {
                return EnumHelper<Detelecine>.GetEnumDisplayValues(typeof(Detelecine));
            }

            if (value is IEnumerable<Denoise>)
            {
                return EnumHelper<Denoise>.GetEnumDisplayValues(typeof(Denoise));
            }

            if (value is IEnumerable<VideoScaler>)
            {
                return EnumHelper<VideoScaler>.GetEnumDisplayValues(typeof(VideoScaler));
            }

            if (value is IEnumerable<OutputFormat>)
            {
                return EnumHelper<OutputFormat>.GetEnumDisplayValues(typeof(OutputFormat));
            }

            if (value is IEnumerable<DeinterlaceFilter>)
            {
                return EnumHelper<DeinterlaceFilter>.GetEnumDisplayValues(typeof(DeinterlaceFilter));
            }

            if (value is IEnumerable<CombDetect>)
            {
                return EnumHelper<CombDetect>.GetEnumDisplayValues(typeof(CombDetect));
            }

            if (value is IEnumerable<Sharpen>)
            {
                return EnumHelper<Sharpen>.GetEnumDisplayValues(typeof(Sharpen));
            }

            if (value is IEnumerable<FileOverwriteBehaviour>)
            {
                return EnumHelper<FileOverwriteBehaviour>.GetEnumDisplayValues(typeof(FileOverwriteBehaviour));
            }

            if (value is IEnumerable<AutonameFileCollisionBehaviour>)
            {
                return EnumHelper<AutonameFileCollisionBehaviour>.GetEnumDisplayValues(typeof(AutonameFileCollisionBehaviour));
            }

            if (value is IEnumerable<WhenDone>)
            {
                return EnumHelper<WhenDone>.GetEnumDisplayValues(typeof(WhenDone));
            }

            // Single Items
            if (targetType == typeof(VideoEncoder) || value.GetType() == typeof(VideoEncoder))
            {
                return EnumHelper<VideoEncoder>.GetDisplay((VideoEncoder)value);
            }

            if (targetType == typeof(PresetPictureSettingsMode) || value.GetType() == typeof(PresetPictureSettingsMode))
            {
                return EnumHelper<PresetPictureSettingsMode>.GetDisplay((PresetPictureSettingsMode)value);
            }

            if (targetType == typeof(Detelecine) || value.GetType() == typeof(Detelecine))
            {
                return EnumHelper<Detelecine>.GetDisplay((Detelecine)value);
            }

            if (targetType == typeof(Denoise) || value.GetType() == typeof(Denoise))
            {
                return EnumHelper<Denoise>.GetDisplay((Denoise)value);
            }

            if (targetType == typeof(QueueItemStatus) || value.GetType() == typeof(QueueItemStatus))
            {
                return EnumHelper<QueueItemStatus>.GetDisplay((QueueItemStatus)value);
            }

            if (targetType == typeof(VideoScaler) || value.GetType() == typeof(VideoScaler))
            {
                return EnumHelper<VideoScaler>.GetDisplay((VideoScaler)value);
            }

            if (targetType == typeof(OutputFormat) || value.GetType() == typeof(OutputFormat))
            {
                return EnumHelper<OutputFormat>.GetDisplay((OutputFormat)value);
            }

            if (targetType == typeof(DeinterlaceFilter) || value.GetType() == typeof(DeinterlaceFilter))
            {
                return EnumHelper<DeinterlaceFilter>.GetDisplay((DeinterlaceFilter)value);
            }

            if (targetType == typeof(CombDetect) || value.GetType() == typeof(CombDetect))
            {
                return EnumHelper<CombDetect>.GetDisplay((CombDetect)value);
            }

            if (targetType == typeof(Sharpen) || value.GetType() == typeof(Sharpen))
            {
                return EnumHelper<Sharpen>.GetDisplay((Sharpen)value);
            }

            if (targetType == typeof(FileOverwriteBehaviour) || value.GetType() == typeof(FileOverwriteBehaviour))
            {
                return EnumHelper<FileOverwriteBehaviour>.GetDisplay((FileOverwriteBehaviour)value);
            }

            if (targetType == typeof(AutonameFileCollisionBehaviour) || value.GetType() == typeof(AutonameFileCollisionBehaviour))
            {
                return EnumHelper<AutonameFileCollisionBehaviour>.GetDisplay((AutonameFileCollisionBehaviour)value);
            }

            if (targetType == typeof(WhenDone) || value.GetType() == typeof(WhenDone))
            {
                return EnumHelper<WhenDone>.GetDisplay((WhenDone)value);
            }

            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is null)
            {
                return null;
            }

            if (targetType == typeof(VideoEncoder) || value.GetType() == typeof(VideoEncoder))
            {
                return EnumHelper<VideoEncoder>.GetValue(value.ToString());
            }

            if (targetType == typeof(PresetPictureSettingsMode) || value.GetType() == typeof(PresetPictureSettingsMode))
            {
                return EnumHelper<PresetPictureSettingsMode>.GetValue(value.ToString());
            }

            if (targetType == typeof(Denoise) || value.GetType() == typeof(Denoise))
            {
                return EnumHelper<Denoise>.GetValue(value.ToString());
            }

            if (targetType == typeof(Detelecine) || value.GetType() == typeof(Detelecine))
            {
                return EnumHelper<Detelecine>.GetValue(value.ToString());
            }

            if (targetType == typeof(VideoScaler) || value.GetType() == typeof(VideoScaler))
            {
                return EnumHelper<VideoScaler>.GetValue(value.ToString());
            }

            if (targetType == typeof(OutputFormat) || value.GetType() == typeof(OutputFormat))
            {
                return EnumHelper<OutputFormat>.GetValue(value.ToString());
            }

            if (targetType == typeof(DeinterlaceFilter) || value.GetType() == typeof(DeinterlaceFilter))
            {
                return EnumHelper<DeinterlaceFilter>.GetValue(value.ToString());
            }

            if (targetType == typeof(CombDetect) || value.GetType() == typeof(CombDetect))
            {
                return EnumHelper<CombDetect>.GetValue(value.ToString());
            }

            if (targetType == typeof(Sharpen) || value.GetType() == typeof(Sharpen))
            {
                return EnumHelper<Sharpen>.GetValue(value.ToString());
            }

            if (targetType == typeof(WhenDone) || value.GetType() == typeof(WhenDone))
            {
                return EnumHelper<WhenDone>.GetValue(value.ToString());
            }

            return null;
        }
    }
}
