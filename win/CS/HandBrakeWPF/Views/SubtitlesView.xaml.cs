// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitlesView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for SubtitlesView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;
    using System.Windows.Controls;

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
            this.InitializeComponent();
        }

        private void SubtitleOptionsButton_OnClick(object sender, RoutedEventArgs e)
        {
            var button = sender as FrameworkElement;
            if (button != null && button.ContextMenu != null)
            {
                button.ContextMenu.PlacementTarget = button;
                button.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
                button.ContextMenu.IsOpen = true;
            }
        }
    }
}
