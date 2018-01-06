// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Filters.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The filter.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Encode
{
    using System.Collections.Generic;

    /// <summary>
    /// The filter.
    /// </summary>
    public class Filters
    {
        /// <summary>
        /// Gets or sets the filter list.
        /// </summary>
        public List<Filter> FilterList { get; set; }
    }
}
