// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DisplayStringAttribute.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DisplayStringAttribute type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    using System;

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
