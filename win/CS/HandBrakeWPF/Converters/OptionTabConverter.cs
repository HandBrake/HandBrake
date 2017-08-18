// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionTabConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Controls display of tab pages
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters
{
    using System;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Utilities;

    public class OptionTabConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            OptionsTab[] tabs = value as OptionsTab[];
            if (tabs != null && UwpDetect.IsUWP())
            {
                return tabs.Where(s => s != OptionsTab.Updates).ToArray();
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
