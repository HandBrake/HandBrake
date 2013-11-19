// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBConfiguration.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrakes Configuration options
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model
{
    /// <summary>
    /// HandBrakes configuration options
    /// </summary>
    public class HBConfiguration
    {
        /// <summary>
        /// Gets or sets a value indicating whether is logging enabled.
        /// </summary>
        public bool IsLoggingEnabled { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether is dvd nav disabled.
        /// </summary>
        public bool IsDvdNavDisabled { get; set; }
    }
}
