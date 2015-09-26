// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EnumToDescConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Enum to Description Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Converters
{
    using System;
    using System.ComponentModel;

    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Utilities;

    /// <summary>
    /// Enum to Description Converter
    /// </summary>
    public class EnumToDescConverter : EnumConverter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="EnumToDescConverter"/> class.
        /// </summary>
        /// <param name="type">
        /// The type.
        /// </param>
        public EnumToDescConverter(Type type)
            : base(type)
        {
        }

        /// <summary>
        /// Convert To an Object.
        /// </summary>
        /// <param name="context">
        /// The context.
        /// </param>
        /// <param name="culture">
        /// The culture.
        /// </param>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <param name="destinationType">
        /// The destination type.
        /// </param>
        /// <returns>
        /// The Enum Object
        /// </returns>
        public override object ConvertTo(
            ITypeDescriptorContext context,
            System.Globalization.CultureInfo culture,
            object value,
            Type destinationType)
        {
            return EnumHelper<Enum>.GetDescription((Enum)value);
        }
    }
}
