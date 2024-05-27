// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IsDefaultModesConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Subtitles
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Model.Subtitles;

    public class IsDefaultModesConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value != null && value.GetType() == typeof(BindingList<IsDefaultModes>))
            {
                return
                    new BindingList<string>(
                        EnumHelper<IsDefaultModes>.GetEnumDisplayValues(typeof(IsDefaultModes)).ToList());
            }

            if (value != null && value.GetType() == typeof(IsDefaultModes))
            {
                return EnumHelper<IsDefaultModes>.GetDisplay((IsDefaultModes)value);
            }

            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string name = value as string;
            if (!string.IsNullOrEmpty(name))
            {
                return EnumHelper<IsDefaultModes>.GetValue(name);
            }

            return null;
        }
    }
}
