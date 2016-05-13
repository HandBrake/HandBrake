﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetTune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PresetTune type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Filters
{
    /// <summary>
    /// The preset tune.
    /// </summary>
    public class PresetTune
    {
        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the short name.
        /// </summary>
        public string Short_Name { get; set; }
    }
}
