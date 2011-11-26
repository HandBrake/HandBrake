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
    using System.ComponentModel;
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
        public static readonly DependencyProperty QueryProperty = DependencyProperty.Register("SubtitleTracks", typeof(BindingList<SubtitleTrack>), typeof(SubtitlesView), new PropertyMetadata(null));

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public BindingList<SubtitleTrack> AudioTracks
        {
            get { return (BindingList<SubtitleTrack>)this.GetValue(QueryProperty); }
            set { this.SetValue(QueryProperty, value); }
        }
    }
}
