// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitlesView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for SubtitlesView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Controls
{
    using System;
    using System.Collections.ObjectModel;
    using System.Windows;
    using System.Windows.Controls;

    using HandBrake.ApplicationServices.Model.Encoding;

    /// <summary>
    /// Interaction logic for SubtitlesView.xaml
    /// </summary>
    public partial class SubtitlesView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitlesView"/> class.
        /// </summary>
        public SubtitlesView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// The "Query" Dependancy Property
        /// </summary>
        public static readonly DependencyProperty SubtitleTracksProperty = DependencyProperty.Register("SubtitleTracks", typeof(ObservableCollection<SubtitleTrack>), typeof(SubtitlesView));

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public ObservableCollection<SubtitleTrack> SubtitleTracks
        {
            get { return (ObservableCollection<SubtitleTrack>)this.GetValue(SubtitleTracksProperty); }
            set { this.SetValue(SubtitleTracksProperty, value); }
        }

        /// <summary>
        /// Add a new Track
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        public void Add(object sender, RoutedEventArgs e)
        {
            this.SubtitleTracks.Add(new SubtitleTrack());
        }

        /// <summary>
        /// Remove a Track
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        public void Remove(object sender, RoutedEventArgs e)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Import an SRT File.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        public void ImportSrt(object sender, RoutedEventArgs e)
        {
            throw new NotImplementedException();
        }
    }
}
