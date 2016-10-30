// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MainView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for MainView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// Interaction logic for MainView.xaml
    /// </summary>
    public partial class MainView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="MainView"/> class.
        /// </summary>
        public MainView()
        {
            this.InitializeComponent();
        }

        /// <summary>
        /// Hide the overflow control on the Preset panel.
        /// TODO find a better way of doing this. This seems to be the common solution. 
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

        /// <summary>
        /// Add to Queue button context menu handling.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void AddToQueue_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            // If we've clicked the dropdown part of the button, display the context menu below the button.
            Button button = (sender as Button);
            if (button != null)
            {
                HitTestResult result = VisualTreeHelper.HitTest(button, e.GetPosition(button));
                FrameworkElement element = result.VisualHit as FrameworkElement;
                if (element != null)
                {
                    if (element.Name == "dropdown" || element.Name == "dropdownArrow")
                    {
                        button.ContextMenu.IsEnabled = true;
                        button.ContextMenu.PlacementTarget = button;
                        button.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
                        button.ContextMenu.IsOpen = true;
                        return;
                    }
                }
            }

            // Otherwise assume it's a main area click and add to queue.
            ((IMainViewModel)this.DataContext).AddToQueue();
        }
    }
}
