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
    public class SourceGeometry
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SourceGeometry"/> class.
        /// </summary>
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