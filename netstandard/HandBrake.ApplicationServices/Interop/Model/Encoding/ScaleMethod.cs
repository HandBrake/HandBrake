// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ScaleMethod.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Scaling Method
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    /// <summary>
    /// Enumeration of rescaling algorithms.
    /// </summary>
    public enum ScaleMethod
    {
        /// <summary>
        /// Standard software scaling. Highest quality.
        /// </summary>
        Lanczos = 0,

        /// <summary>
        /// OpenCL-assisted bicubic scaling.
        /// </summary>
        Bicubic = 1
    }
}
