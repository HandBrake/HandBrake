// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InterfaceLanguage.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Languages supported by the UI.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    public class InterfaceLanguage
    {
        public InterfaceLanguage(string culture, string name)
        {
            this.Culture = culture;
            this.Name = name;
        }

        public string Culture { get; set; }
        public string Name { get; set; }

        protected bool Equals(InterfaceLanguage other)
        {
            return string.Equals(this.Culture, other.Culture);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((InterfaceLanguage)obj);
        }

        public override int GetHashCode()
        {
            return (this.Culture != null ? this.Culture.GetHashCode() : 0);
        }
    }
}
