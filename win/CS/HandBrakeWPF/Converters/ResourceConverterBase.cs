// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ResourceConverterBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ResourceConverterBase type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Converters
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;

    using HandBrake.App.Core.Utilities;

    public class ResourceConverterBase<T>
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
            if (value != null && value.GetType() == typeof(BindingList<T>))
            {
                return
                    new BindingList<string>(
                        EnumHelper<T>.GetEnumDisplayValuesSubset((BindingList<T>)value).ToList());
            }

            if (value != null && value.GetType() == typeof(T))
            {
                return EnumHelper<T>.GetDisplay((T)value);
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
                return EnumHelper<T>.GetValue(name);
            }

            return null;
        }
    }
}
