/*  EnumToDescConveter.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */


namespace HandBrake.ApplicationServices.Converters
{
    using System;
    using System.ComponentModel;

    using HandBrake.ApplicationServices.Functions;

    /// <summary>
    /// Enum to Description Converter
    /// </summary>
    public class EnumToDescConveter : EnumConverter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="EnumToDescConveter"/> class.
        /// </summary>
        /// <param name="type">
        /// The type.
        /// </param>
        public EnumToDescConveter(Type type)
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
