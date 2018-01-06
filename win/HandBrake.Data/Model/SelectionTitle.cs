// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SelectionTitle.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A model for the multiple selection window for adding to the queue.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using Caliburn.Micro;

    using HandBrakeWPF.Services.Scan.Model;

    /// <summary>
    /// A model for the multiple selection window for adding to the queue.
    /// </summary>
    public class SelectionTitle : PropertyChangedBase
    {
        /// <summary>
        /// The source name.
        /// </summary>
        private readonly string sourceName;

        /// <summary>
        /// The is selected.
        /// </summary>
        private bool isSelected;

        /// <summary>
        /// Initializes a new instance of the <see cref="SelectionTitle"/> class.
        /// </summary>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="sourceName">
        /// The source Name.
        /// </param>
        public SelectionTitle(Title title, string sourceName)
        {
            this.sourceName = sourceName;
            this.Title = title;
        }

        /// <summary>
        /// Gets the source name.
        /// </summary>
        public string SourceName
        {
            get
            {
                return !string.IsNullOrEmpty(Title.SourceName) ? Title.SourceName : sourceName;
            }
        }

        /// <summary>
        /// Gets or sets the end point.
        /// </summary>
        public int EndPoint { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether is selected.
        /// </summary>
        public bool IsSelected
        {
            get
            {
                return this.isSelected;
            }
            set
            {
                this.isSelected = value;
                this.NotifyOfPropertyChange(() => this.IsSelected);
            }
        }

        /// <summary>
        /// Gets or sets the start point.
        /// </summary>
        public int StartPoint { get; set; }

        /// <summary>
        /// Gets or sets the title.
        /// </summary>
        public Title Title { get; set; }
    }
}