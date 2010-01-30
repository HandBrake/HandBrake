/*  Preset.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;

namespace Handbrake.Presets
{
    public class Preset
    {
        /// <summary>
        /// Get or Set the category which the preset resides under
        /// </summary>
        public string Category { get; set; }

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