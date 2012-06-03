/*  ChapterMarker.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using Caliburn.Micro;

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
        /// <param name="Name">
        /// The name.
        /// </param>
        public ChapterMarker(int number, string Name)
        {
            this.ChapterName = Name;
            this.ChapterNumber = number;
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
        }

        /// <summary>
        /// Gets or sets The number of this Chapter, in regards to it's parent Title
        /// </summary>
        public int ChapterNumber { get; set; }

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
