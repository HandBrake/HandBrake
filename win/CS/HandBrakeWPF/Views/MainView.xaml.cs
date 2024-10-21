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
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.ViewModels;
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
            ((IMainViewModel)this.DataContext).AddToQueueWithErrorHandling();
        }

        private void Presets_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            // Focus the keyboard navigation onto the floating panel and show it. 
            KeyboardNavigation.SetTabNavigation(this.mainBodyGrid, ((MainViewModel)this.DataContext).IsPresetPaneDisplayed ? KeyboardNavigationMode.Continue : KeyboardNavigationMode.None);
            ((MainViewModel)this.DataContext).TogglePresetPane();
        }

        private void TabControl_OnSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (e.Source is TabControl && e.AddedItems.Count > 0)
            {
                TabItem tab = e.AddedItems[0] as TabItem;
                if (tab != null && Properties.Resources.MainView_SummaryTab.Equals(tab.Header))
                {
                    ((MainViewModel)this.DataContext).SummaryViewModel.UpdateDisplayedInfo();
                }

                this.tabControl.Focus();
            }
        }
        
        private void SelectPreset_OnClick(object sender, RoutedEventArgs e)
        {
            var button = sender as FrameworkElement;
            if (button != null && button.ContextMenu != null)
            {
                button.ContextMenu.PlacementTarget = button;
                button.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Right;
                button.ContextMenu.IsOpen = true;
            }
        }
        
        private void ParentGrid_OnPreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            bool isPresetPaneDisplayed = ((MainViewModel)this.DataContext).IsPresetPaneDisplayed;

            if (isPresetPaneDisplayed)
            {
                HitTestResult presetControlPressed = VisualTreeHelper.HitTest(presetPaneControl, e.GetPosition(presetPaneControl));
                HitTestResult presetButtonPressed = VisualTreeHelper.HitTest(presetbtn, e.GetPosition(presetbtn));

                if (presetButtonPressed == null && presetControlPressed == null)
                {
                    KeyboardNavigation.SetTabNavigation(this.mainBodyGrid, KeyboardNavigationMode.Continue);
                    ((MainViewModel)this.DataContext).TogglePresetPane(); // Clicked off the preset control
                }
            }
        }

        private void InfoButton_OnPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            // If we've clicked the dropdown part of the button, display the context menu below the button.
            Button button = (sender as Button);
            if (button != null)
            {
                HitTestResult result = VisualTreeHelper.HitTest(button, e.GetPosition(button));
                FrameworkElement element = result.VisualHit as FrameworkElement;
                if (element != null)
                {
                    button.ContextMenu.IsEnabled = true;
                    button.ContextMenu.PlacementTarget = button;
                    button.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
                    button.ContextMenu.IsOpen = true;
                }
            }
        }
    }
}
