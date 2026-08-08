// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FilterSettingsDescConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Queue
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Windows.Data;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model;

    public class FilterSettingsDescConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            EncodeTask task = value as EncodeTask;
            if (task != null)
            {
                List<string> filters = new List<string>();

                foreach (var filter in task.VideoFilters)
                {
                    filters.Add(filter.DisplayName);
                }

                if (task.Rotation != 0 || task.FlipVideo)
                {
                    filters.Add(Resources.SummaryView_Rotation);
                }

                if (filters.Count == 0)
                {
                    return Resources.SummaryView_NoFilters;
                }
                
                return string.Join(", ", filters).TrimEnd(',').Trim();
            }

            return Resources.SummaryView_NoFilters;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
