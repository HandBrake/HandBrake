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
    /// The Source Type. (File, Folder, DVD)
    /// </summary>
    public enum SourceType
    {
        /// <summary>
        /// None
        /// </summary>
        None = 0,

        /// <summary>
        /// File
        /// </summary>
        File,

        /// <summary>
        /// Video Folder
        /// </summary>
        VideoFolder,

        /// <summary>
        /// DVD
        /// </summary>
        Dvd
    }
}