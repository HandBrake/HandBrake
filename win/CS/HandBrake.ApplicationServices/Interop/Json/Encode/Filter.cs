// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Filter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The filter list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Encode
{
    /// <summary>
    /// The filter list.
    /// </summary>
    public class Filter
    {
        /// <summary>
        /// Gets or sets the id.
        /// </summary>
        public int ID { get; set; }

        /// <summary>
        /// Gets or sets the settings.
        /// </summary>
        public string Settings { get; set; }
    }
}