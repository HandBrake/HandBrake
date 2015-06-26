// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetTransportContainer.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preset transport container.
//   This is a model for importing the JSON / Plist presets into the GUI.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Presets
{
    using System.Collections.Generic;

    /// <summary>
    /// The preset transport container.
    /// This is a model for importing the JSON / Plist presets into the GUI.
    /// </summary>
    public class PresetTransportContainer
    {
        /// <summary>
        /// Gets or sets the children array.
        /// </summary>
        public List<HBPreset> PresetList { get; set; }

        /// <summary>
        /// Gets or sets the version major.
        /// </summary>
        public string VersionMajor { get; set; }

        /// <summary>
        /// Gets or sets the version micro.
        /// </summary>
        public string VersionMicro { get; set; }

        /// <summary>
        /// Gets or sets the version minor.
        /// </summary>
        public string VersionMinor { get; set; }
    }
}
