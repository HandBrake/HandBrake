// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetTransportContainer.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preset transport container.
//   This is a model for importing the JSON presets into the GUI.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Presets
{
    using System.Collections.Generic;

    /// <summary>
    /// The preset transport container.
    /// This is a model for importing the JSON presets into the GUI.
    /// </summary>
    public class PresetTransportContainer
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PresetTransportContainer"/> class.
        /// </summary>
        public PresetTransportContainer()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PresetTransportContainer"/> class.
        /// </summary>
        /// <param name="versionMajor">
        /// The version major.
        /// </param>
        /// <param name="versionMinor">
        /// The version minor.
        /// </param>
        /// <param name="versionMicro">
        /// The version micro.
        /// </param>
        public PresetTransportContainer(int versionMajor, int versionMinor, int versionMicro)
        {
            this.VersionMajor = versionMajor;
            this.VersionMicro = versionMicro;
            this.VersionMinor = versionMinor;
        }

        /// <summary>
        /// Gets or sets the children array.
        /// </summary>
        public List<object> PresetList { get; set; }

        /// <summary>
        /// Gets or sets the version major.
        /// </summary>
        public int VersionMajor { get; set; }

        /// <summary>
        /// Gets or sets the version micro.
        /// </summary>
        public int VersionMicro { get; set; }

        /// <summary>
        /// Gets or sets the version minor.
        /// </summary>
        public int VersionMinor { get; set; }
    }
}
