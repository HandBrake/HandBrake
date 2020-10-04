// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HbPresetCategory.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preset category.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Presets
{
    using System.Collections.Generic;

    public class HBPresetCategory
    {
        public List<HBPreset> ChildrenArray { get; set; }

        public bool Folder { get; set; }

        public string PresetName { get; set; }

        public string PresetDescription { get; set; }

        public int Type { get; set; }
    }
}