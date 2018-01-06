// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Chapter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object representing a Chapter aosciated with a Title, in a DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Model
{
    using System;
    using System.Globalization;

    /// <summary>
    /// An object representing a Chapter aosciated with a Title, in a DVD
    /// </summary>
    public class Chapter
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Chapter"/> class.
        /// </summary>
        public Chapter()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Chapter"/> class.
        /// </summary>
        /// <param name="number">
        /// The number.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <param name="duration">
        /// The duration.
        /// </param>
        public Chapter(int number, string name, TimeSpan duration)
        {
            this.ChapterName = name;
            this.ChapterNumber = number;
            this.Duration = duration;
        }

        /// <summary>
        /// Gets or sets The number of this Chapter, in regards to it's parent Title
        /// </summary>
        public int ChapterNumber { get; set; }

        /// <summary>
        /// Gets or sets ChapterName.
        /// </summary>
        public string ChapterName { get; set; }

        /// <summary>
        /// Gets or sets The length in time this Chapter spans
        /// </summary>
        public TimeSpan Duration { get; set; }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {chapter #}</returns>
        public override string ToString()
        {
            return this.ChapterNumber.ToString(CultureInfo.InvariantCulture);
        }
    }
}