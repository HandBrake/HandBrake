// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for LogView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;
    using System.Windows.Controls;

    /// <summary>
    /// Interaction logic for LogView.xaml
    /// </summary>
    public partial class LogView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LogView"/> class.
        /// </summary>
        public LogView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Hide the Toolbar Endplate.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ToolBarLoaded(object sender, RoutedEventArgs e)
        {
            ToolBar toolBar = sender as ToolBar;
            if (toolBar != null)
            {
                var overflowGrid = toolBar.Template.FindName("OverflowGrid", toolBar) as FrameworkElement;
                if (overflowGrid != null)
                {
                    overflowGrid.Visibility = Visibility.Collapsed;
                }
            }
        }
    }
}
