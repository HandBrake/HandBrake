// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoRangeType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the VideoRangeType type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The video range type.
    /// </summary>
    public enum VideoRangeType
    {
        /// <summary>
        /// The chapters.
        /// </summary>
        [Display(Name = "Chapters")]
        Chapters, 

        /// <summary>
        /// The seconds.
        /// </summary>
        [Display(Name = "Seconds")]
        Seconds, 

        /// <summary>
        /// The frames.
        /// </summary>
        [Display(Name = "Frames")]
        Frames
    }
}