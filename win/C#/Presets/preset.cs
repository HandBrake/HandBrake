using System;

namespace Handbrake.Presets
{
    public class Preset
    {
        /// <summary>
        /// Get or Set the preset's level. This indicated if it is a root or child node
        /// </summary>
        public int Level { get; set; }

        /// <summary>
        /// Get or Set the category which the preset resides under
        /// </summary>
        public string Category { get; set; }

        /// <summary>
        /// Get or Set the top level category for the preset.
        /// </summary>
        public string TopCategory { get; set; }

        /// <summary>
        /// Get or Set the preset name
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Get or set the preset query
        /// </summary>
        public string Query { get; set; }

        /// <summary>
        /// Get or set the usage of Picture Settings in presets.
        /// </summary>
        public Boolean PictureSettings { get; set; }

        /// <summary>
        /// The version number which associates this preset with a HB build
        /// </summary>
        public string Version { get; set; }
    }
}