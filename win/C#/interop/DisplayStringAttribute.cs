using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    [AttributeUsage(AttributeTargets.Field)]
    public sealed class DisplayStringAttribute : Attribute
    {
        private readonly string value;

        public string Value
        {
            get { return value; }
        }

        public string ResourceKey { get; set; }

        public DisplayStringAttribute(string v)
        {
            this.value = v;
        }

        public DisplayStringAttribute()
        {
        }
    }
}
