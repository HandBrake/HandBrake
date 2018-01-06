// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AnamorphicGeometry.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The geometry.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Anamorphic
{
    using HandBrake.ApplicationServices.Interop.Json.Shared;

    /// <summary>
    /// The geometry.
    /// </summary>
    public class AnamorphicGeometry
    {
        /// <summary>
        /// Gets or sets the dest geometry.
        /// </summary>
        public DestSettings DestSettings { get; set; }

        /// <summary>
        /// Gets or sets the source geometry.
        /// </summary>
        public Geometry SourceGeometry { get; set; }
    }
}