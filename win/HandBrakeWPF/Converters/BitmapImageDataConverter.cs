// --------------------------------------------------------------------------------------------------------------------
// <copyright file="BitmapImageDataConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Converts ImageData into a BitmapImage.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Converters
{
    using System;
    using System.Globalization;
    using System.Windows.Data;
    using HandBrake.CoreLibrary.Model;
    using HandBrake.Utilities;

    public class BitmapImageDataConverter : IValueConverter
    {
        private ImageData data;

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            this.data = (ImageData)value;

            if (this.data != null)
            {
                return BitmapUtilities.ConvertToBitmapImage(this.data);
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return this.data;
        }
    }
}