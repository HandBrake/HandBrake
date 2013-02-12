// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the SourceType type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    /// <summary>
    /// The source type.
    /// </summary>
    public enum SourceType
    {
        /// <summary>
        /// The none.
        /// </summary>
        None = 0, 

        /// <summary>
        /// The file.
        /// </summary>
        File, 

        /// <summary>
        /// The video folder.
        /// </summary>
        VideoFolder, 

        /// <summary>
        /// The dvd.
        /// </summary>
        Dvd
    }
}