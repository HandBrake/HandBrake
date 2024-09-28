// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Source.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The source.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Encode
{
    /// <summary>
    /// The source.
    /// </summary>
    public class Source
    {
        /// <summary>
        /// Gets or sets the angle.
        /// </summary>
        public int Angle { get; set; }

        /// <summary>
        /// Gets or sets the range.
        /// </summary>
        public Range Range { get; set; }

        /// <summary>
        /// Gets or sets the title.
        /// </summary>
        public int Title { get; set; }

        /// <summary>
        /// Gets or sets the path.
        /// </summary>
        public string Path { get; set; }

        public int HWDecode { get; set; }

        public bool KeepDuplicateTitles { get; set; }
    }
}