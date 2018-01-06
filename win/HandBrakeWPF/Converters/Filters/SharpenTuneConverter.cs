﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SharpenTuneConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A converter to fetch the sharpen tunes
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Filters
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Windows.Data;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    using HandBrakeWPF.Model.Filters;

    public class SharpenTuneConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
        {
            if (values.Length == 2)
            {
                Sharpen selectedSharpen = (Sharpen)values[1];
                if (selectedSharpen == Sharpen.LapSharp)
                {
                    BindingList<FilterTune> tunes = new BindingList<FilterTune>();
                    foreach (HBPresetTune tune in HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_LAPSHARP))
                    {
                        tunes.Add(new FilterTune(tune));
                    }
                    return tunes;
                }
                else if (selectedSharpen == Sharpen.UnSharp)
                {
                    BindingList<FilterTune> tunes = new BindingList<FilterTune>();
                    foreach (HBPresetTune tune in HandBrakeFilterHelpers.GetFilterTunes((int)hb_filter_ids.HB_FILTER_UNSHARP))
                    {
                        tunes.Add(new FilterTune(tune));
                    }
                    return tunes;
                }
            }

            return new BindingList<HBPresetTune>();
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
