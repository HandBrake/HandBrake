// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioAdvancedView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for AudioAdvancedView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;
    using System.Windows.Controls;

    /// <summary>
    /// Interaction logic for AudioAdvancedView.xaml
    /// </summary>
    public partial class AudioAdvancedView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AudioAdvancedView"/> class.
        /// </summary>
        public AudioAdvancedView()
        {
            InitializeComponent();
        }

        private void OptionsButton_OnClick(object sender, RoutedEventArgs e)
        {
            var button = sender as FrameworkElement;
            if (button != null && button.ContextMenu != null)
            {
                button.ContextMenu.PlacementTarget = button;
                button.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
                button.ContextMenu.IsOpen = true;
            }
        }

        private void ListViewItem_PreviewMouseRightButtonDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            ListViewItem item = sender as ListViewItem;
            if (item != null && !item.IsSelected)
            {
                item.IsSelected = true;
            }
        }
    }
}
