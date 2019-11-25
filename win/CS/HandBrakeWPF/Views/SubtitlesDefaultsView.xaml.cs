// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitlesDefaultsView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for SubtitlesDefaultsView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;
    using System.Windows.Controls;

    using HandBrakeWPF.ViewModels;

    /// <summary>
    /// Interaction logic for SubtitlesDefaultsView.xaml
    /// </summary>
    public partial class SubtitlesDefaultsView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SubtitlesDefaultsView"/> class.
        /// </summary>
        public SubtitlesDefaultsView()
        {
            this.InitializeComponent();
        }

        private void Apply_OnClick(object sender, RoutedEventArgs e)
        {
            ((SubtitlesDefaultsViewModel)DataContext).IsApplied = true;
            this.Close();
        }
    }
}
