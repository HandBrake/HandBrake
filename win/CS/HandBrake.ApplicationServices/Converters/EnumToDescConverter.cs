namespace HandBrake.ApplicationServices.Converters
{
    using System.ComponentModel;
    using System;

    using HandBrake.ApplicationServices.Functions;

    public class EnumToDescConveter : EnumConverter
    {
        public EnumToDescConveter(Type type)
            : base(type)
        {
        }

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
