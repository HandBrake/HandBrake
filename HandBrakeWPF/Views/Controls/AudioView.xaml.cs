// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for AudioView.xaml
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
    /// Interaction logic for AudioView.xaml
    /// </summary>
    public partial class AudioView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AudioView"/> class.
        /// </summary>
        public AudioView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// The "Query" Dependancy Property
        /// </summary>
        public static readonly DependencyProperty AudioTracksProperty = DependencyProperty.Register("AudioTracks", typeof(ObservableCollection<AudioTrack>), typeof(AudioView));

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public ObservableCollection<AudioTrack> AudioTracks
        {
            get { return (ObservableCollection<AudioTrack>)this.GetValue(AudioTracksProperty); }
            set { this.SetValue(AudioTracksProperty, value); }
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
            this.AudioTracks.Add(new AudioTrack());
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
    }
}
