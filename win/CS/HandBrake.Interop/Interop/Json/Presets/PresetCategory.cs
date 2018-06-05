// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetCategory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preset category.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Presets
{
    using System.Collections.Generic;

    /// <summary>
    /// The preset category.
    /// </summary>
    public class PresetCategory
    {
        /// <summary>
        /// Gets or sets the children array.
        /// </summary>
        public List<HBPreset> ChildrenArray { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether folder.
        /// </summary>
        public bool Folder { get; set; }

        /// <summary>
        /// Gets or sets the preset name.
        /// </summary>
        public string PresetName { get; set; }

        /// <summary>
        /// Description for the preset group.
        /// </summary>
        public string PresetDescription { get; set; }

        /// <summary>
        /// Gets or sets the type.
        /// </summary>
        public int Type { get; set; }
    }
}