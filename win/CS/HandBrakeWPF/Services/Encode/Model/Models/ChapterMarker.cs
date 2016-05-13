// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChapterMarker.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Movie Chapter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System;

    using HandBrakeWPF.Utilities;

    /// <summary>
    /// A Movie Chapter
    /// </summary>
    public class ChapterMarker : PropertyChangedBase
    {
        /// <summary>
        /// Backing field for chapter name
        /// </summary>
        private string chapterName;

        /// <summary>
        /// Initializes a new instance of the <see cref="ChapterMarker"/> class.
        /// </summary>
        public ChapterMarker()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ChapterMarker"/> class.
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
        public ChapterMarker(int number, string name, TimeSpan duration)
        {
            this.ChapterName = name;
            this.ChapterNumber = number;
            this.Duration = duration;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ChapterMarker"/> class.
        /// Copy Constructor
        /// </summary>
        /// <param name="chapter">
        /// The chapter.
        /// </param>
        public ChapterMarker(ChapterMarker chapter)
        {
            this.ChapterName = chapter.ChapterName;
            this.ChapterNumber = chapter.ChapterNumber;
            this.Duration = chapter.Duration;
        }

        /// <summary>
        /// Gets or sets The number of this Chapter, in regards to it's parent Title
        /// </summary>
        public int ChapterNumber { get; set; }

        /// <summary>
        /// Gets or sets the duration.
        /// </summary>
        public TimeSpan Duration { get; set; }

        /// <summary>
        /// Gets or sets ChapterName.
        /// </summary>
        public string ChapterName
        {
            get
            {
                return this.chapterName;
            }
            set
            {
                this.chapterName = value;
                this.NotifyOfPropertyChange(() => this.ChapterName);
            }
        }
    }
}
