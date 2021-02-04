// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FilterTune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Preset Tune
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Filters
{
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;

    public class FilterTune
    {
        public FilterTune()
        {
        }

        public FilterTune(string displayName, string key)
        {
            this.DisplayName = displayName;
            this.Key = key;
        }

        public FilterTune(HBPresetTune presetTune)
        {
            this.DisplayName = presetTune?.Name;
            this.Key = presetTune?.ShortName;
        }

        public string DisplayName { get; set; }
        public string Key { get; set; }

        protected bool Equals(FilterTune other)
        {
            return string.Equals(this.Key, other.Key);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((FilterTune)obj);
        }

        public override int GetHashCode()
        {
            return this.Key != null ? this.Key.GetHashCode() : 0;
        }
    }
}
