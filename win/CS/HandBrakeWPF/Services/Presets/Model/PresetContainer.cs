// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetContainer.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A container object for presets. This object should not change often as it's designed for preset version tracking.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets.Model
{
    public class PresetContainer
    {
        public PresetContainer(int version, string presets)
        {
            Version = version;
            Presets = presets;
        }

        /// <summary>
        /// Gets or sets the version of the presets stored in this container.
        /// </summary>
        public int Version { get; set; }

        /// <summary>
        /// Gets or sets the presets. This is a serialised string.
        /// </summary>
        public string Presets { get; set;  }
    }
}
