// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Cropping.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Cropping type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    /// <summary>
    /// Cropping T B L R
    /// </summary>
    public class Cropping
    {
        /// <summary>
        /// Gets or sets Top.
        /// </summary>
        public int Top { get; set; }

        /// <summary>
        /// Gets or sets Bottom.
        /// </summary>
        public int Bottom { get; set; }

        /// <summary>
        /// Gets or sets Left.
        /// </summary>
        public int Left { get; set; }

        /// <summary>
        /// Gets or sets Right.
        /// </summary>
        public int Right { get; set; }
    }
}
