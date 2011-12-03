/*  ChapterMarker.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// A Movie Chapter
    /// </summary>
    public class ChapterMarker
    {
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
        /// Gets or sets The number of this Chapter, in regards to it's parent Title
        /// </summary>
        public int ChapterNumber { get; set; }

        /// <summary>
        /// Gets or sets ChapterName.
        /// </summary>
        public string ChapterName { get; set; }
    }
}
