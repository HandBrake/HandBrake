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

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Model.Filters;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModelItems.Filters;

    public class FilterSettingsDescConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            EncodeTask task = value as EncodeTask;
            if (task != null)
            {
                List<string> filters = new List<string>();

                if (task.Detelecine != Detelecine.Off)
                {
                    filters.Add(Resources.SummaryView_Detelecine);
                }

                if (task.DeinterlaceFilter != DeinterlaceFilter.Off)
                {
                    filters.Add(EnumHelper<DeinterlaceFilter>.GetShortName(task.DeinterlaceFilter));
                }

                if (task.Denoise != Denoise.Off)
                {
                    filters.Add(task.Denoise.ToString());
                }

                if (task.Sharpen != Sharpen.Off)
                {
                    filters.Add(task.Sharpen.ToString());
                }

                if (task.DeblockPreset != null && task.DeblockPreset.Key != DeblockFilter.Off)
                {
                    filters.Add(Resources.SummaryView_Deblock);
                }

                if (task.Grayscale)
                {
                    filters.Add(Resources.SummaryView_Grayscale);
                }

                if (task.Rotation != 0 || task.FlipVideo)
                {
                    filters.Add(Resources.SummaryView_Rotation);
                }

                if (task.Colourspace != null && task.Colourspace.Key != ColourSpaceFilter.Off)
                {
                    filters.Add(Resources.SummaryView_Colourspace);
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
