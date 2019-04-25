// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SharpenPresetConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A converter to fetch the sharpen presets
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Filters
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Windows.Data;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Model.Encoding;

    using HandBrakeWPF.Model.Filters;

    public class SharpenPresetConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            if (values.Length == 2)
            {
                Sharpen selectedSharpen = (Sharpen)values[1];
                if (selectedSharpen == Sharpen.LapSharp)
                {
                    BindingList<FilterPreset> presets = new BindingList<FilterPreset>();
                    foreach (HBPresetTune preset in HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_LAPSHARP))
                    {
                        presets.Add(new FilterPreset(preset));
                    }

                    return presets;
                }
                else if (selectedSharpen == Sharpen.UnSharp)
                {
                    BindingList<FilterPreset> presets = new BindingList<FilterPreset>();
                    foreach (HBPresetTune preset in HandBrakeFilterHelpers.GetFilterPresets((int)hb_filter_ids.HB_FILTER_UNSHARP))
                    {
                        presets.Add(new FilterPreset(preset));
                    }

                    return presets;
                }
            }

            return new BindingList<FilterPreset>();
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
