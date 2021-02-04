// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetVersion.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PresetVersion type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Presets
{
    public class PresetVersion
    {
        public PresetVersion(int major, int minor, int micro)
        {
            this.Major = major;
            this.Minor = minor;
            this.Micro = micro;
        }

        public int Major { get; }

        public int Minor { get; }

        public int Micro { get; }
    }
}
