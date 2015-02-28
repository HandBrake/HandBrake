// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceGeometry.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The source geometry.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Anamorphic
{
    /// <summary>
    /// The source geometry.
    /// </summary>
    internal class SourceGeometry
    {
        public SourceGeometry()
        {
            this.PAR = new PAR();
        }

        /// <summary>
        /// Gets or sets the height.
        /// </summary>
        public int Height { get; set; }

        /// <summary>
        /// Gets or sets the par.
        /// </summary>
        public PAR PAR { get; set; }

        /// <summary>
        /// Gets or sets the width.
        /// </summary>
        public int Width { get; set; }
    }
}