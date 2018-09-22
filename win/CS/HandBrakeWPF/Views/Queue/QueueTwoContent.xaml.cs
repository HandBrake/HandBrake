// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueTwoView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for QueueTwoView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Queue
{
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.ViewModels;

    /// <summary>
    /// Interaction logic for VideoView
    /// </summary>
    public partial class QueueTwoContent : UserControl
    {
        private QueueTask mouseActiveQueueTask;

        /// <summary>
        /// Initializes a new instance of the <see cref="Queue2View"/> class.
        /// </summary>
        public QueueTwoContent()
        {
            this.InitializeComponent();
            this.SizeChanged += this.Queue2View_SizeChanged;
        }

        private void Queue2View_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            // Make the view adaptive. 
            if (e.WidthChanged)
            {
                this.summaryTabControl.Visibility = this.ActualWidth < 550 ? Visibility.Collapsed : Visibility.Visible;
                this.leftTabPanel.Width = this.ActualWidth < 550 ? new GridLength(this.ActualWidth - 10, GridUnitType.Star) : new GridLength(3, GridUnitType.Star);
                this.leftTabPanel.MaxWidth = this.ActualWidth < 550 ? 550 : 400;
            }
        }
        private void ContextMenu_OnOpened(object sender, RoutedEventArgs e)
        {
            ContextMenu menu = sender as ContextMenu;
            this.mouseActiveQueueTask = null;

            if (menu != null)
            {
                Point p = Mouse.GetPosition(this);
                HitTestResult result = VisualTreeHelper.HitTest(this, p);

                if (result != null)
                {
                    ListBoxItem listBoxItem = FindParent<ListBoxItem>(result.VisualHit);
                    if (listBoxItem != null)
                    {
                        this.mouseActiveQueueTask = listBoxItem.DataContext as QueueTask;
                    }
                }
            }

            this.openSourceDir.IsEnabled = this.mouseActiveQueueTask != null;
            this.openDestDir.IsEnabled = this.mouseActiveQueueTask != null;
        }

        private static T FindParent<T>(DependencyObject from) where T : class
        {
            DependencyObject parent = VisualTreeHelper.GetParent(from);

            T result = null;
            if (parent is T)
            {
                result = parent as T;
            }
            else if (parent != null)
            {
                result = FindParent<T>(parent);
            }

            return result;
        }

        private void OpenSourceDir_OnClick(object sender, RoutedEventArgs e)
        {
            ((QueueViewModel)this.DataContext).OpenSourceDirectory(this.mouseActiveQueueTask);
        }

        private void OpenDestDir_OnClick(object sender, RoutedEventArgs e)
        {
            ((QueueViewModel)this.DataContext).OpenDestinationDirectory(this.mouseActiveQueueTask);
        }
    }
}
