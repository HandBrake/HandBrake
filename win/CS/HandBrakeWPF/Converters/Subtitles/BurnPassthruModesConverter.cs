// --------------------------------------------------------------------------------------------------------------------
// <copyright file="BurnPassthruModesConverter.cs" company="HandBrake Project (http://handbrake.fr)">
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

    public class BurnPassthruModesConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value != null && value.GetType() == typeof(BindingList<SubtitleBurnInBehaviourModes>))
            {
                return
                    new BindingList<string>(
                        EnumHelper<SubtitleBurnInBehaviourModes>.GetEnumDisplayValues(typeof(SubtitleBurnInBehaviourModes)).ToList());
            }

            if (value != null && value.GetType() == typeof(SubtitleBurnInBehaviourModes))
            {
                return EnumHelper<SubtitleBurnInBehaviourModes>.GetDisplay((SubtitleBurnInBehaviourModes)value);
            }

            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string name = value as string;
            if (!string.IsNullOrEmpty(name))
            {
                return EnumHelper<SubtitleBurnInBehaviourModes>.GetValue(name);
            }

            return null;
        }
    }
}
