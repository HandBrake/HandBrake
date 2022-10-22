// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CropModeConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>

// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Picture
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Model.Picture;

    public class CropModeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value != null && value.GetType() == typeof(BindingList<CropMode>))
            {
                return
                    new BindingList<string>(
                        EnumHelper<CropMode>.GetEnumDisplayValues(typeof(CropMode)).ToList());
            }

            if (value != null && value.GetType() == typeof(CropMode))
            {
                return EnumHelper<CropMode>.GetDisplay((CropMode)value);
            }

            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string name = value as string;
            if (!string.IsNullOrEmpty(name))
            {
                return EnumHelper<CropMode>.GetValue(name);
            }

            return null;
        }
    }
}
