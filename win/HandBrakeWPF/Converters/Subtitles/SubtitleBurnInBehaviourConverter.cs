// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleBurnInBehaviourConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Subtitle Behaviour Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters.Subtitles
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Data;

    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Utilities;

    /// <summary>
    /// Subtitle Behaviour Converter
    /// </summary>
    public class SubtitleBurnInBehaviourConverter : IValueConverter
    {
        /// <summary>
        /// The convert.
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
