// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PictureSettingsDescConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PictureSettingsDescConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Queue
{
    using System;
    using System.Globalization;
    using System.Windows.Data;

    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    using HandBrakeWPF.Services.Encode.Model;

    /// <summary>
    /// The picture settings desc converter.
    /// </summary>
    public class PictureSettingsDescConverter : IValueConverter
    {
        /// <summary>
        /// Provides a textual description of the picture settings of an encode task.
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
        /// The <see cref="object"/>.
        /// </returns>
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            EncodeTask task = value as EncodeTask;
            if (task != null)
            {
                string resolution = string.Empty;
                switch (task.Anamorphic)
                {
                    case Anamorphic.Automatic:
                        resolution = "Anamorphic: Automatic";
                        break;
                    case Anamorphic.Custom:
                        resolution = "Anamorphic: Custom, Resolution: " + task.Width + "x" + task.Height;
                        break;
                    case Anamorphic.None:
                        resolution = "Resolution: " + task.Width + "x" + task.Height;
                        break;
                }

                return resolution + Environment.NewLine + "Crop Top: " + task.Cropping.Top + ", Bottom: " + task.Cropping.Bottom + ", Left: "
                       + task.Cropping.Left + ", Right: " + task.Cropping.Right;
            }

            return string.Empty;
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
        /// The <see cref="object"/>.
        /// </returns>
        /// <exception cref="NotImplementedException">
        /// Not Used
        /// </exception>
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
