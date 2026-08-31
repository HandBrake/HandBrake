// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioFilter.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio filter.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Presets
{
    /// <summary>
    /// The audio filter.
    /// </summary>
    public class AudioFilter
    {
        /// <summary>
        /// Gets or sets the audio filter custom settings.
        /// </summary>
        public string AudioFilterCustom { get; set; }

        /// <summary>
        /// Gets or sets the audio filter (short) name.
        /// </summary>
        public string AudioFilterName { get; set; }

        /// <summary>
        /// Gets or sets the audio filter preset.
        /// </summary>
        public string AudioFilterPreset { get; set; }

        /// <summary>
        /// Gets or sets the audio filter tune.
        /// </summary>
        public string AudioFilterTune { get; set; }
    }
}
