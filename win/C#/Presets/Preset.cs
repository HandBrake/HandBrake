/*  Preset.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Presets
{
    /// <summary>
    /// A Preset
    /// </summary>
    public class Preset 
    {
        /// <summary>
        /// Gets or sets the category which the preset resides under
        /// </summary>
        public string Category { get; set; }

        /// <summary>
        /// Gets or sets the preset name
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the preset query
        /// </summary>
        public string Query { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to use picture Settings in presets.
        /// </summary>
        public bool CropSettings { get; set; }

        /// <summary>
        /// Gets or sets The version number which associates this preset with a HB build
        /// </summary>
        public string Version { get; set; }

        /// <summary>
        /// Gets or sets the Description for the preset
        /// </summary>
        public string Description { get; set; }
    }
}