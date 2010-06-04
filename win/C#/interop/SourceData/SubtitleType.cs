// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the SubtitleType type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.SourceData
{
    /// <summary>
    /// Subtitle Type. 
    /// 0: Picture
    /// 1: Text
    /// </summary>
    public enum SubtitleType
    {
        /// <summary>
        /// Picture Subtitle
        /// </summary>
        Picture,

        /// <summary>
        /// Text Subtitle
        /// </summary>
        Text
    }
}
