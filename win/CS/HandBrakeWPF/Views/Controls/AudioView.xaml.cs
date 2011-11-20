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
    using System.ComponentModel;
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
            this.AudioTracks = new BindingList<AudioTrack>();
        }

        /// <summary>
        /// The "Query" Dependancy Property
        /// </summary>
        public static readonly DependencyProperty QueryProperty = DependencyProperty.Register("AudioTracks", typeof(BindingList<AudioTrack>), typeof(AudioView), new PropertyMetadata(null));


        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public BindingList<AudioTrack> AudioTracks
        {
            get { return (BindingList<AudioTrack>)this.GetValue(QueryProperty); }
            set { this.SetValue(QueryProperty, value); }
        }

        /// <summary>
        /// Add an audio track.
        /// </summary>
        public void Add()
        {
            this.AudioTracks.Add(new AudioTrack());
        }

        /// <summary>
        /// Remove an Audio Track
        /// </summary>
        public void Remove()
        {
            throw new NotImplementedException("Not Done Yet!");
        }
    }
}
