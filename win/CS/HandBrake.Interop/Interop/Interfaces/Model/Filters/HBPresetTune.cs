// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBPresetTune.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object representing the key and name of a Filter Preset or Tune option.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Filters
{
    public class HBPresetTune
    {
        public static string Custom = "custom";
        public static string None = "none";

        public HBPresetTune()
        {
        }

        public HBPresetTune(string name, string shortName)
        {
            this.Name = name;
            this.ShortName = shortName;
        }

        public string Name { get; set; }

        public string ShortName { get; set; }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj.GetType() != this.GetType())
            {
                return false;
            }

            return this.Equals((HBPresetTune)obj);
        }

        public override int GetHashCode()
        {
            return this.ShortName != null ? this.ShortName.GetHashCode() : 0;
        }

        protected bool Equals(HBPresetTune other)
        {
            return string.Equals(this.ShortName, other.ShortName);
        }
    }
}
