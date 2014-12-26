// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AnamorphicGeometry.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The geometry.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Json.Anamorphic
{
    /// <summary>
    /// The geometry.
    /// </summary>
    internal class AnamorphicGeometry
    {
        /// <summary>
        /// Gets or sets the dest geometry.
        /// </summary>
        public DestSettings DestGeometry { get; set; }

        /// <summary>
        /// Gets or sets the source geometry.
        /// </summary>
        public SourceGeometry SourceGeometry { get; set; }
    }
}