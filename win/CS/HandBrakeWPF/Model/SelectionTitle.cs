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
    using System;
    using System.IO;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Scan.Model;
    using HandBrakeWPF.ViewModels;

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
                return !string.IsNullOrEmpty(Title.SourcePath) ? Title.SourcePath : sourceName;
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

        public string SourceInfo => string.Format("{0}{1}", FilesizeStr, SourceInfoHelper.GenerateSourceInfo(this.Title));

        public decimal? FilesizeMB => CalcFilesize();

        public string FilesizeStr
        {
            get
            {
                if (FilesizeMB.HasValue)
                {
                    return string.Format("{0} MB, ", FilesizeMB);
                }

                return null;
            }
        }

        private decimal? CalcFilesize()
        {
            if (this.Title != null && !string.IsNullOrEmpty(this.Title.SourcePath) && File.Exists(this.Title.SourcePath))
            {
                if (Path.GetExtension(this.Title.SourcePath)?.Contains("iso", StringComparison.InvariantCultureIgnoreCase) ?? false)
                {
                    return null;
                }

                FileInfo info = new FileInfo(this.Title.SourcePath);
                return Math.Round((decimal)info.Length / 1024 / 1024, 1);
            }

            return null;
        }
    }
}