// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AnamorphicResult.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The anamorphic result.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Anamorphic
{
    /// <summary>
    /// The anamorphic result.
    /// </summary>
    public class AnamorphicResult
    {
        /// <summary>
        ///     Gets or sets the height.
        /// </summary>
        public int Height { get; set; }

        /// <summary>
        ///     Gets or sets the par.
        /// </summary>
        public PAR PAR { get; set; }

        /// <summary>
        ///     Gets or sets the width.
        /// </summary>
        public int Width { get; set; }
    }
}