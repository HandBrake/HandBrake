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
    /// <summary>
    /// The preset container.
    /// </summary>
    public class PresetContainer
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PresetContainer"/> class.
        /// </summary>
        /// <param name="version">
        /// The version.
        /// </param>
        /// <param name="presets">
        /// The presets.
        /// </param>
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
