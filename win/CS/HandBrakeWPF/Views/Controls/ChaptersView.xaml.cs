// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChaptersView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for ChaptersView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Controls
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Windows;
    using System.Windows.Controls;

    using HandBrake.ApplicationServices.Model.Encoding;

    /// <summary>
    /// Interaction logic for ChaptersView.xaml
    /// </summary>
    public partial class ChaptersView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ChaptersView"/> class.
        /// </summary>
        public ChaptersView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// The "Chapters" Dependancy Property
        /// </summary>
        public static readonly DependencyProperty ChaptersProperty = DependencyProperty.Register("Chapters", typeof(ObservableCollection<ChapterMarker>), typeof(ChaptersView));

        /// <summary>
        /// The "EnableChapterMarkers" Dependancy Property
        /// </summary>
        public static readonly DependencyProperty IncludeChaptersProperty = DependencyProperty.Register("IncludeChapterMarkers", typeof(bool), typeof(ChaptersView));

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public IList<ChapterMarker> Chapters
        {
            get { return (ObservableCollection<ChapterMarker>)this.GetValue(ChaptersProperty); }
            set { this.SetValue(ChaptersProperty, value); }
        }

        /// <summary>
        /// Gets or sets a value indicating whether chapter markers are enabled.
        /// </summary>
        public bool IncludeChapterMarkers
        {
            get { return (bool)this.GetValue(IncludeChaptersProperty); }
            set { this.SetValue(IncludeChaptersProperty, value); }
        }

        /// <summary>
        /// Import from CSV
        /// </summary>
        public void Import()
        {
            throw new NotImplementedException("Not Implemented Yet");
        }

        /// <summary>
        /// Export to CSV
        /// </summary>
        public void Export()
        {
            throw new NotImplementedException("Not Implemented Yet");
        }
    }
}
