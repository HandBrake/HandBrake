// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Mp4Options.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The mp 4 options.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Encode
{
    /// <summary>
    /// The mp 4 options.
    /// </summary>
    public class Mp4Options
    {
        /// <summary>
        /// Gets or sets a value indicating whether ipod atom.
        /// </summary>
        public bool IpodAtom { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether mp 4 optimize.
        /// </summary>
        public bool Mp4Optimize { get; set; }
    }
}